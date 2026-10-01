const PROFILES = new Set(["ori", "eitan"]);
const SUBJECTS = new Set(["math", "hebrew", "english", "religion"]);
const MAX_RESULTS = 64;

function json(payload, status = 200) {
  return Response.json(payload, {
    status,
    headers: {
      "Cache-Control": "no-store",
      "Content-Security-Policy": "default-src 'none'",
      "X-Content-Type-Options": "nosniff",
    },
  });
}

function authorized(request, env) {
  const auth = request.headers.get("Authorization") || "";
  return Boolean(env.DEVICE_SYNC_TOKEN) && auth === ("Bearer " + env.DEVICE_SYNC_TOKEN);
}

function validDate(value) {
  if (typeof value !== "string" || !/^\d{4}-\d{2}-\d{2}$/.test(value)) return false;
  const d = new Date(value + "T00:00:00Z");
  return !Number.isNaN(d.getTime()) && d.toISOString().slice(0, 10) === value;
}

function normalizeResult(item) {
  if (!item || typeof item !== "object") return null;
  const profileKey = String(item.profileKey || "").trim().toLowerCase();
  const activityDate = String(item.activityDate || "").trim();
  const subject = String(item.subject || "").trim().toLowerCase();
  const correctFirstTry = Number(item.correctFirstTry);
  const sourceRevision = Number(item.sourceRevision == null ? correctFirstTry : item.sourceRevision);
  if (!PROFILES.has(profileKey) || !validDate(activityDate) || !SUBJECTS.has(subject)) return null;
  if (!Number.isInteger(correctFirstTry) || correctFirstTry < 0 || correctFirstTry > 65535) return null;
  if (!Number.isInteger(sourceRevision) || sourceRevision < 0 || sourceRevision > 2147483647) return null;
  return { profileKey, activityDate, subject, correctFirstTry, sourceRevision };
}

async function creditDeviceAggregate(env, result) {
  const noonUtcMs = Date.parse(result.activityDate + "T12:00:00Z");
  if (!Number.isFinite(noonUtcMs)) return 0;

  const link = await env.RESULTS_DB
    .prepare("SELECT child_id, family_id, enabled_from_ms FROM learning_profile_links WHERE profile_key = ?")
    .bind(result.profileKey)
    .first();

  if (!link || noonUtcMs < Number(link.enabled_from_ms)) return 0;

  let credited = 0;
  for (let ordinal = 1; ordinal <= result.correctFirstTry; ordinal++) {
    const syncId = [
      "ahava_device",
      result.profileKey,
      result.activityDate,
      result.subject,
      String(ordinal),
    ].join(":");

    const txId = "academy:" + syncId;
    const questionId = "device:" + result.subject + ":" + ordinal;

    const write = await env.RESULTS_DB
      .prepare(`
        INSERT OR IGNORE INTO learning_reward_credits (
          sync_id, profile_key, child_id, family_id, subject, question_id,
          question_timestamp_ms, activity_date, transaction_id
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
      `)
      .bind(
        syncId,
        result.profileKey,
        link.child_id,
        link.family_id,
        result.subject,
        questionId,
        noonUtcMs + ordinal,
        result.activityDate,
        txId
      )
      .run();

    if (Number(write?.meta?.changes || 0) > 0) credited += 1;
  }

  return credited;
}

async function handleBatch(request, env) {
  if (!authorized(request, env)) return json({ error: "unauthorized" }, 401);
  if (!env.RESULTS_DB) return json({ error: "database_unavailable" }, 503);

  let body;
  try { body = await request.json(); }
  catch { return json({ error: "invalid_json" }, 400); }

  const deviceId = typeof body.deviceId === "string" ? body.deviceId.trim() : "";
  if (!/^[A-Za-z0-9._-]{1,64}$/.test(deviceId)) return json({ error: "invalid_device_id" }, 400);
  if (!Array.isArray(body.results) || body.results.length === 0 || body.results.length > MAX_RESULTS) {
    return json({ error: "invalid_results" }, 400);
  }

  const normalized = [];
  for (const item of body.results) {
    const parsed = normalizeResult(item);
    if (!parsed) return json({ error: "invalid_result_item" }, 400);
    normalized.push(parsed);
  }

  const sql = [
    "INSERT INTO learning_results_device_daily",
    "(profile_key, activity_date, subject, correct_first_try, device_id, source_revision, updated_at)",
    "VALUES (?1, ?2, ?3, ?4, ?5, ?6, strftime('%Y-%m-%dT%H:%M:%fZ','now'))",
    "ON CONFLICT(profile_key, activity_date, subject) DO UPDATE SET",
    "correct_first_try = MAX(learning_results_device_daily.correct_first_try, excluded.correct_first_try),",
    "device_id = excluded.device_id,",
    "source_revision = MAX(learning_results_device_daily.source_revision, excluded.source_revision),",
    "updated_at = strftime('%Y-%m-%dT%H:%M:%fZ','now')"
  ].join(" ");

  const statements = normalized.map((r) =>
    env.RESULTS_DB.prepare(sql).bind(
      r.profileKey, r.activityDate, r.subject, r.correctFirstTry, deviceId, r.sourceRevision
    )
  );

  await env.RESULTS_DB.batch(statements);

  let minutesCredited = 0;
  for (const result of normalized) {
    minutesCredited += await creditDeviceAggregate(env, result);
  }

  return json({
    ok: true,
    source: "ahava_device",
    accepted: normalized.length,
    minutesCredited,
    serverTime: new Date().toISOString(),
  });
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (request.method === "GET" && url.pathname === "/health") {
      return json({ ok: true, service: "ahava-device-results-api" });
    }
    if (request.method === "POST" && url.pathname === "/v1/results/batch") {
      return handleBatch(request, env);
    }
    return json({ error: "not_found" }, 404);
  },
};
