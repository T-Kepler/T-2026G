# 板载50MHz基准时钟和双路比较器最高65MHz测频输入。
create_clock -name CLK_50M -period 20.000 [get_ports {CLK}]
create_clock -name AD1_INPUT_CLK -period 15.385 [get_ports {AD1_INPUT_CLK}]
create_clock -name AD2_INPUT_CLK -period 15.385 [get_ports {AD2_INPUT_CLK}]

# MYPLL: c0=150MHz系统时钟，c1=125MHz DA时钟（数据与CLK同源，匹配AD9767官方方案）。
derive_pll_clocks
derive_clock_uncertainty

# DA数据在c1下降沿更新，AD9767由c1上升沿采样。数据更新到采样间隔4ns。
# 输出时钟从引脚到达AD9767，输出时钟路径也必须计入关系。
set da_clock_source [get_clock_info -targets \
    [get_clocks {pll|altpll_component|auto_generated|pll1|clk[2]}]]
create_generated_clock -name DA1_CAPTURE_CLOCK -source $da_clock_source [get_ports {DA1_OUTCLK}]
create_generated_clock -name DA2_CAPTURE_CLOCK -source $da_clock_source [get_ports {DA2_OUTCLK}]
set_output_delay -clock DA1_CAPTURE_CLOCK -max 2.000 [get_ports {DA1_OUT[*]}]
set_output_delay -clock DA1_CAPTURE_CLOCK -min -1.500 [get_ports {DA1_OUT[*]}]
set_output_delay -clock DA2_CAPTURE_CLOCK -max 2.000 [get_ports {DA2_OUT[*]}]
set_output_delay -clock DA2_CAPTURE_CLOCK -min -1.500 [get_ports {DA2_OUT[*]}]

# DA配置遵循“停止输出-更新寄存器-重新启动”的稳定多位CDC协议。
set_false_path -to [get_registers {da_enable_meta}]
set_false_path -to [get_registers {*wave_select_da*}]
set_false_path -to [get_registers {*|frequency_word_a[*] *|frequency_word_b[*]}]
set_false_path -to [get_registers {*|phase_offset_a[*] *|phase_offset_b[*]}]
set_false_path -to [get_registers {*|voltage_meta[*]}]

# 比较器测频控制的两级同步器只豁免第一级，第二级及计数器路径仍正常分析。
set_false_path -to [get_registers {*count_enable_meta *count_clear_meta}]

# FMC读事务异步进入clk_base域，只豁免两级同步器的第一级。
set_false_path -to [get_registers {*ad1_fifo_read_meta}]

# STM32停止测频并等待1ms后才读取；此时跨入FMC读缓存的32位计数总线保持稳定。
set_false_path -from [get_registers {*frequency_counter|Q1[*]}] \
    -to [get_registers {*fmc|rd_data_reg[*]}]
