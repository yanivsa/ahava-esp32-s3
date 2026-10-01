CREATE TABLE IF NOT EXISTS learning_results_daily (
  profile_key TEXT NOT NULL CHECK (profile_key IN ('ori','eitan')),
  activity_date TEXT NOT NULL,
  source TEXT NOT NULL CHECK (source IN ('ahava_device','ahava_app')),
  subject TEXT NOT NULL CHECK (subject IN ('math','hebrew','english','religion')),
  correct_first_try INTEGER NOT NULL DEFAULT 0 CHECK (correct_first_try >= 0),
  device_id TEXT,
  source_revision INTEGER NOT NULL DEFAULT 0 CHECK (source_revision >= 0),
  updated_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
  PRIMARY KEY (profile_key, activity_date, source, subject)
);

CREATE INDEX IF NOT EXISTS idx_learning_results_profile_date
  ON learning_results_daily (profile_key, activity_date DESC);

CREATE INDEX IF NOT EXISTS idx_learning_results_source_date
  ON learning_results_daily (source, activity_date DESC);

DROP VIEW IF EXISTS learning_results_combined_daily;
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
