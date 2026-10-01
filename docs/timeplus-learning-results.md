# Ahava learning-results cloud contract

This infrastructure exposes results only. It does not unify or synchronize
question banks between the ESP32 device and the Ahava web/Android application.

D1 database: timeplus-db

Table: learning_results_daily

Stable identity fields:
- profile_key: ori or eitan
- activity_date: local Israel calendar date in YYYY-MM-DD
- source: ahava_device or ahava_app
- subject: math, hebrew, english, religion

Metric:
- correct_first_try: absolute daily number of first-try correct answers

The primary key is profile_key + activity_date + source + subject. Device retries
are idempotent because the cloud receives an absolute value and uses UPSERT,
never a blind increment.

Device endpoint:
POST https://ahava-device-results-api.yanivsa.workers.dev/v1/results/batch

Authentication uses a bearer token stored as a Cloudflare Worker secret and
injected into firmware by the GitHub Actions secret AHAVA_RESULTS_SYNC_TOKEN.

The device sends no question IDs and no question text. It sends only daily
first-try-correct counts for Ori and Eitan. The current firmware snapshot covers
the latest seven calendar days and is resent safely when needed.

Time+ does not need any firmware API change. Its backend can later read D1
directly. For source-separated data, read learning_results_daily. For a combined
App + Device total, read learning_results_combined_daily. The view keeps both
correct_from_device and correct_from_app, and also exposes their sum as
correct_first_try_total.

Mapping profile_key values to Time+ child records is intentionally left to the
Time+ project.
