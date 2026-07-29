"""连续分级采集必须在每轮开始前清空对应FPGA FIFO。"""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


if __name__ == "__main__":
    fifo = (ROOT / "zuolan_FPGA/ip/TYFIFO/TYFIFO.v").read_text(encoding="utf-8")
    top = (ROOT / "zuolan_FPGA/src/TOP.v").read_text(encoding="utf-8")
    fmc = (ROOT / "zuolan_FPGA/src/FMC_CONTROL.v").read_text(encoding="utf-8")
    adc = (ROOT / "zuolan_stm32/MY_Hardware_Drivers/Src/ad_measure.c").read_text(encoding="utf-8")

    assert "input\t  aclr;" in fifo and ".aclr (aclr)" in fifo
    assert ".aclr  (control_data[14])" in top
    assert ".aclr  (control_data[15])" in top
    assert "fifo_read_data" in fmc
    assert "write_data_6_" in fmc and "write_data_8_" in fmc
    assert "fmc_rd_en ? (fifo_read_address ? fifo_read_data : rd_data_reg)" in fmc
    assert "resetFifo(channel);" in adc
    assert "resetFifo(1);" in adc and "resetFifo(2);" in adc
    assert "cycles_exact" in adc
    assert "ad_capture_g_frame" in adc
    assert "G_CAPTURE_CTRL = G_CAPTURE_LONG_AD1" in adc
    assert "ADC_FIFO_READ_GAP_NOPS 16U" in adc
    assert "static void waitForFifoAdvance(void)" in adc
    assert adc.count("waitForFifoAdvance();") == 2
    assert "__NOP();" in adc
    print("Capture contract passed: FIFO reset and G long-frame capture are active")
