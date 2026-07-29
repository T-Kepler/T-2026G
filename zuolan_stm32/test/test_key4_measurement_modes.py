"""检查G题四键映射、消抖和主循环事件处理。"""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


if __name__ == "__main__":
    key = (ROOT / "zuolan_stm32/MY_Hardware_Drivers/Src/key_app.c").read_text(
        encoding="utf-8")
    app = (ROOT / "zuolan_stm32/MY_APP/g_signal_app.c").read_text(
        encoding="utf-8")
    header = (ROOT / "zuolan_stm32/MY_APP/g_signal_app.h").read_text(
        encoding="utf-8")
    scheduler = (ROOT / "zuolan_stm32/MY_APP/scheduler.c").read_text(
        encoding="utf-8")

    mappings = (
        ("KEY1_Pin", "G_SIGNAL_CONTROL_MODE_NEXT"),
        ("KEY2_Pin", "G_SIGNAL_CONTROL_PERIOD_TOGGLE"),
        ("KEY3_Pin", "G_SIGNAL_CONTROL_VIEW_TOGGLE"),
        ("KEY4_Pin", "G_SIGNAL_CONTROL_MEASURE"),
    )
    for pin, control in mappings:
        assert pin in key and control in key

    assert "KEY_DEBOUNCE_MS 30U" in key
    assert "GSignalApp_HandleControl(keys[index].control)" in key
    assert "typedef enum" in header
    assert "GSignalApp_Task" in scheduler
    assert "current_mode + 1U" in app
    assert "display_periods == 1U" in app
    assert "G_VIEW_SPECTRUM" in app
    assert "G_APP_REFRESH_MS" not in app
    assert "last_measurement_ms" not in app
    assert "performMeasurement();" in app
    assert "(events & G_EVENT_MEASURE)" in app
    print("G key modes passed: mode, periods, view and one-shot measurement")
