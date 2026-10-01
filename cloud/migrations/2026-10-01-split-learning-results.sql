-- One-time migration: split legacy source-tagged results into two physical tables.
CREATE TABLE IF NOT EXISTS learning_results_device_daily (
  profile_key TEXT NOT NULL CHECK (profile_key IN ('ori','eitan')),
  activity_date TEXT NOT NULL,
  subject TEXT NOT NULL CHECK (subject IN ('math','hebrew','english','religion')),
  correct_first_try INTEGER NOT NULL DEFAULT 0 CHECK (correct_first_try >= 0),
  device_id TEXT NOT NULL,
  source_revision INTEGER NOT NULL DEFAULT 0 CHECK (source_revision >= 0),
  updated_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
  PRIMARY KEY (profile_key, activity_date, subject)
);

CREATE TABLE IF NOT EXISTS learning_results_app_daily (
  profile_key TEXT NOT NULL CHECK (profile_key IN ('ori','eitan')),
  activity_date TEXT NOT NULL,
  subject TEXT NOT NULL CHECK (subject IN ('math','hebrew','english','religion')),
  correct_first_try INTEGER NOT NULL DEFAULT 0 CHECK (correct_first_try >= 0),
  source_revision INTEGER NOT NULL DEFAULT 0 CHECK (source_revision >= 0),
  updated_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
  PRIMARY KEY (profile_key, activity_date, subject)
);

INSERT INTO learning_results_device_daily
  (profile_key, activity_date, subject, correct_first_try, device_id, source_revision, updated_at)
SELECT profile_key, activity_date, subject, correct_first_try,
       COALESCE(device_id, 'legacy-device'), source_revision, updated_at
FROM learning_results_daily
WHERE source='ahava_device'
ON CONFLICT(profile_key, activity_date, subject) DO UPDATE SET
  correct_first_try = MAX(learning_results_device_daily.correct_first_try, excluded.correct_first_try),
  device_id = excluded.device_id,
  source_revision = MAX(learning_results_device_daily.source_revision, excluded.source_revision),
  updated_at = MAX(learning_results_device_daily.updated_at, excluded.updated_at);

INSERT INTO learning_results_app_daily
  (profile_key, activity_date, subject, correct_first_try, source_revision, updated_at)
SELECT profile_key, activity_date, subject, correct_first_try, source_revision, updated_at
FROM learning_results_daily
WHERE source='ahava_app'
ON CONFLICT(profile_key, activity_date, subject) DO UPDATE SET
  correct_first_try = MAX(learning_results_app_daily.correct_first_try, excluded.correct_first_try),
  source_revision = MAX(learning_results_app_daily.source_revision, excluded.source_revision),
  updated_at = MAX(learning_results_app_daily.updated_at, excluded.updated_at);

DROP VIEW IF EXISTS learning_results_combined_daily;
ALTER TABLE learning_results_daily RENAME TO learning_results_daily_legacy_20261001;

CREATE INDEX IF NOT EXISTS idx_learning_results_device_profile_date
  ON learning_results_device_daily (profile_key, activity_date DESC);
CREATE INDEX IF NOT EXISTS idx_learning_results_app_profile_date
  ON learning_results_app_daily (profile_key, activity_date DESC);

CREATE VIEW learning_results_daily AS
SELECT profile_key, activity_date, 'ahava_device' AS source, subject,
       correct_first_try, device_id, source_revision, updated_at
FROM learning_results_device_daily
UNION ALL
SELECT profile_key, activity_date, 'ahava_app' AS source, subject,
       correct_first_try, NULL AS device_id, source_revision, updated_at
FROM learning_results_app_daily;

CREATE VIEW learning_results_combined_daily AS
SELECT
  profile_key,
  activity_date,
  subject,
  SUM(correct_first_try) AS correct_first_try_total,
  SUM(CASE WHEN source='ahava_device' THEN correct_first_try ELSE 0 END) AS correct_from_device,
  SUM(CASE WHEN source='ahava_app' THEN correct_first_try ELSE 0 END) AS correct_from_app,
  MAX(updated_at) AS updated_at
FROM learning_results_daily
GROUP BY profile_key, activity_date, subject;

DROP TABLE learning_results_daily_legacy_20261001;
