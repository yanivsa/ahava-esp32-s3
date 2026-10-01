# Ahava learning-results cloud contract

This infrastructure exposes results only. It does not unify or synchronize
question banks between the ESP32 device and the Ahava web/Android application.

D1 database: timeplus-db

## Physical source tables

The two sources are intentionally stored in different physical tables:

- learning_results_device_daily — results uploaded by the ESP32 handheld.
- learning_results_app_daily — results derived from Ahava web/Android cloud snapshots.

Both use stable identity fields: profile_key (ori/eitan), activity_date,
subject, correct_first_try, source_revision. The device table also stores
device_id.

There is no writable source column in either physical table, so device data
cannot accidentally become app data or vice versa.

## Read-only views

- learning_results_daily — compatibility UNION view with source set to
  ahava_device or ahava_app.
- learning_results_combined_daily — one row per child/date/subject with
  correct_from_device, correct_from_app and correct_first_try_total.

Time+ should normally read the two physical tables separately and combine them
only when a product view explicitly wants a total.

## Device upload

Endpoint:
POST https://ahava-device-results-api.yanivsa.workers.dev/v1/results/batch

Authentication uses a bearer token stored as a Cloudflare Worker secret and
injected into firmware by the GitHub Actions secret AHAVA_RESULTS_SYNC_TOKEN.

The device sends no question IDs and no question text. It sends only daily
first-try-correct counts for Ori and Eitan. The Worker writes only to
learning_results_device_daily.

Retries are idempotent: the cloud receives an absolute value and uses UPSERT
with a monotonic floor, never a blind increment.

## App source

Ahava web/Android already records correctFirstTry per question attempt in its
cloud snapshot. The Ahava sync Worker mirrors those absolute daily aggregates
into learning_results_app_daily. This is independent from the ESP32 endpoint.

## Offline / connectivity behavior

- The seven-day statistics UI is not the cloud queue.
- A separate NVS result ledger retains up to 120 calendar days per profile.
- Each subject/day aggregate carries a dirty bit until the server acknowledges it.
- Uploads are sent in batches of up to 64 aggregate rows.
- A successful acknowledgement clears only the exact value sent; a newer local
  increment remains dirty.
- While the handheld is awake, Wi-Fi remains in Station mode with auto-reconnect
  and results_sync_poll runs automatically.
- Before normal deep sleep, firmware performs a best-effort results flush while
  Wi-Fi is still available.
- Pending data stays in NVS if upload is unavailable and retries later.
