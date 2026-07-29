# T-2026G

2026 G题周期信号测量分析装置工程，包含STM32和FPGA源码。

## 功能

- 模式1：10 kHz至200 kHz周期信号测量
- 模式2：10 kHz至500 kHz周期信号测量
- 模式3：有用信号分析及1 MHz、2 MHz数字干扰抑制
- 频率、峰峰值、有效值及最多三个频率分量测量
- 时域波形、频谱、单周期/三周期显示
- VCA810闭环增益控制及分段幅度校准
- 按键单次采样、页面数据保持和一键清空

## 工程结构

- `zuolan_stm32`：STM32F429应用、算法、驱动、HMI和测试
- `zuolan_FPGA`：EP4CE10、AD9226采集及FIFO逻辑

## 关键工程

- Keil：`zuolan_stm32/MDK-ARM/zuolan_STM32.uvprojx`
- STM32CubeMX：`zuolan_stm32/zuolan_STM32.ioc`
- Quartus：`zuolan_FPGA/prj/ZUOLAN_FPGA_OBJECT.qpf`

## 测试

在`zuolan_stm32`目录运行：

```powershell
python test/test_g_signal_design.py
python test/test_g_signal_robustness.py
```

## 硬件配置

- AD9226原始采样率：5 MSPS
- STM32端4倍降采样，有效采样率：1.25 MSPS
- FFT点数：4096，频率分辨率约305.18 Hz
- 信号输入端只保留一个50 ohm终端，避免并联成为25 ohm
