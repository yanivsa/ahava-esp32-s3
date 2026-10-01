from pathlib import Path

root = Path(__file__).resolve().parents[1]
schema = (root / "cloud" / "learning_results_schema.sql").read_text(encoding="utf-8")
worker = (root / "cloud" / "ahava-device-results-worker.js").read_text(encoding="utf-8")
main = (root / "src" / "main.cpp").read_text(encoding="utf-8")
ota = (root / "src" / "engine" / "ota_manager.cpp").read_text(encoding="utf-8")
player = (root / "src" / "engine" / "player_data.cpp").read_text(encoding="utf-8")

for token in [
    "CREATE TABLE IF NOT EXISTS learning_results_device_daily",
    "CREATE TABLE IF NOT EXISTS learning_results_app_daily",
    "CREATE VIEW IF NOT EXISTS learning_results_daily",
    "CREATE VIEW IF NOT EXISTS learning_results_combined_daily",
]:
    assert token in schema, f"missing cloud contract: {token}"

assert "CREATE TABLE IF NOT EXISTS learning_results_daily" not in schema
assert "INSERT INTO learning_results_device_daily" in worker
assert "INSERT INTO learning_results_daily" not in worker
assert "'ahava_device'" in worker
assert "WiFi.setAutoReconnect(true)" in ota
assert "results_sync_poll();" in main
assert "results_sync_store_record_correct(profile, count_date, stats_subject_id);" in player

sleep_start = main.index("static void enter_power_save_sleep")
sleep_end = main.index("static void background_telemetry_task", sleep_start)
sleep_block = main[sleep_start:sleep_end]
assert sleep_block.index("results_sync_poll();") < sleep_block.index("ota_wifi_disconnect();")

print("cloud/results sync contract QA: PASS")
