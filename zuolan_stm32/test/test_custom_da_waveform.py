"""STM32保留四张128点表和双路FPGA波形RAM更新能力。"""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


if __name__ == "__main__":
    source = (ROOT / "zuolan_stm32/MY_Hardware_Drivers/Src/da_output.c").read_text(
        encoding="utf-8")
    header = (ROOT / "zuolan_stm32/MY_Hardware_Drivers/Inc/da_output.h").read_text(
        encoding="utf-8")
    common = (ROOT / "zuolan_stm32/MY_Utilities/Inc/commond_init.h").read_text(
        encoding="utf-8")
    top = (ROOT / "zuolan_FPGA/src/TOP.v").read_text(encoding="utf-8")

    for name in ("sine", "square", "triangle", "deviation"):
        body = re.search(
            rf"da_wave_{name}\[DA_ROM_SIZE\]\s*=\s*\{{(.*?)\}};",
            source,
            re.S,
        ).group(1)
        samples = [int(value) for value in re.findall(r"\d+", body)]
        assert len(samples) == 128, f"{name} table must contain 128 samples"
        assert all(0 <= value <= 16383 for value in samples)
        if name == "deviation":
            assert min(samples) == 1276 and max(samples) == 13445
            assert sum(samples) == 1075202

    assert "WAVE_CUSTOM = 4" in header
    assert "DA_PRESET_DEVIATION" in header and "DA_PRESET_COUNT" in header
    assert "DA_WAVE_RAM(index)" in common and "0x0100U + (index)" in common
    assert "DA_FPGA_STOP();" in source
    assert "DA_WAVE_RAM(i) = samples[i];" in source
    assert top.count("DA_CUSTOM_WAVEFORM_RAM da") == 2
    assert "address[15:7] == 9'h002" in top

    print("Custom DA waveform passed: four STM32 tables can update both FPGA RAMs")
