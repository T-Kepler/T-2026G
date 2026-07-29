// 作者：左岚
// V5 纯 Verilog 顶层。
// 连接关系由原 TOP.bdf 等价迁移，寄存器地址映射保持不变。
module TOP (
    input         CLK,
    input         RST,
    input         FPGA_NL_NADV,
    input         FPGA_WR_NWE,
    input         FPGA_RD_NOE,
    input         FPGA_CS_NEL,
    inout  [15:0] FPGA_DB,
    input  [11:0] AD1_INPUT,
    input  [11:0] AD2_INPUT,
    input         AD1_INPUT_CLK,
    input         AD2_INPUT_CLK,
    output [13:0] DA1_OUT,
    output [13:0] DA2_OUT,
    output        DA1_OUTCLK,
    output        DA2_OUTCLK,
    output        AD1_OUTCLK,
    output        AD2_OUTCLK
);

  wire clk_base;
  wire da_data_clock;
  wire da_capture_clock;
  reg da_enable_meta = 1'b0;
  reg da_enable_sync = 1'b0;
  wire [15:0] address;
  wire fmc_write_enable;
  wire fmc_read_enable;
  wire ad1_fifo_read_active;
  wire ad1_fifo_pop;
  reg ad1_fifo_read_meta = 1'b0;
  reg ad1_fifo_read_sync = 1'b0;
  reg ad1_fifo_read_sync_d = 1'b0;

  wire [15:0] read_data_0;
  wire [15:0] read_data_1;
  wire [15:0] read_data_2;
  wire [15:0] read_data_3;
  wire [15:0] read_data_4;
  wire [15:0] read_data_5;
  wire [15:0] read_data_6;
  wire [15:0] read_data_7;
  wire [15:0] read_data_8;
  wire [15:0] read_data_9;
  wire [15:0] read_data_10;
  wire [15:0] read_data_11;
  wire [15:0] read_data_12;
  wire [15:0] read_data_13;
  wire [15:0] read_data_14;
  wire [15:0] read_data_15;

  wire [15:0] control_data;
  wire [15:0] control_readback;
  wire [15:0] g_control_data;
  wire ad1_long_capture;

  wire [15:0] base1_frequency_high;
  wire [15:0] base1_frequency_low;
  wire [15:0] base2_frequency_high;
  wire [15:0] base2_frequency_low;
  wire [15:0] ad1_frequency_high;
  wire [15:0] ad1_frequency_low;
  wire [15:0] ad2_frequency_high;
  wire [15:0] ad2_frequency_low;
  wire [15:0] ad1_fifo_data_read;
  wire [15:0] ad2_fifo_data_read;
  wire [15:0] ad1_fifo_flag_read;
  wire [15:0] ad2_fifo_flag_read;

  wire [15:0] da1_frequency_word_high;
  wire [15:0] da1_frequency_word_low;
  wire [15:0] da2_frequency_word_high;
  wire [15:0] da2_frequency_word_low;
  wire [11:0] da1_amplitude;
  wire [11:0] da2_amplitude;
  wire [7:0] da1_wave_select;
  wire [7:0] da2_wave_select;
  reg [7:0] da1_wave_select_da = 8'd0;
  reg [7:0] da2_wave_select_da = 8'd0;
  wire [6:0] da1_rom_address;
  wire [6:0] da2_rom_address;
  wire [13:0] da1_wave_data;
  wire [13:0] da2_wave_data;
  wire [13:0] da1_custom_wave_data;
  wire [13:0] da2_custom_wave_data;
  wire custom_wave_write_enable;

  wire [15:0] ad1_sample_word_high;
  wire [15:0] ad1_sample_word_low;
  wire [15:0] ad2_sample_word_high;
  wire [15:0] ad2_sample_word_low;
  wire ad1_sampling_clock_nco;
  wire ad2_sampling_clock_nco;
  wire ad1_sampling_clock;
  wire ad2_sampling_clock;
  wire sync_phase_load;
  wire ad2_capture_strobe;
  reg sync_mode_d = 1'b0;
  reg sync_capture_started = 1'b0;
  reg ad1_sampling_clock_d = 1'b0;
  reg ad2_sampling_clock_d = 1'b0;
  reg ad1_sample_strobe = 1'b0;
  reg ad2_sample_strobe = 1'b0;
  reg [11:0] ad1_input_pipe1 = 12'h800;
  reg [11:0] ad1_input_pipe2 = 12'h800;
  reg [11:0] ad2_input_pipe1 = 12'h800;
  reg [11:0] ad2_input_pipe2 = 12'h800;
  wire [11:0] ad1_fifo_data;
  wire [11:0] ad2_fifo_data;
  wire ad1_fifo_full;
  wire ad2_fifo_full;
  reg [13:0] ad1_capture_count = 14'd0;
  reg [9:0] ad2_capture_count = 10'd0;
  reg ad1_capture_done = 1'b0;
  reg ad2_capture_done = 1'b0;
  wire [31:0] ad1_frequency_count;
  wire [31:0] ad2_frequency_count;
  wire [31:0] base1_frequency_count;
  wire [31:0] base2_frequency_count;

  // ADx_INPUT_CLK来自外部比较器，仅用于独立测频，不参与ADC并行数据采样。
  // 比较器测频控制跨入各自输入时钟域；基准计数器直接使用clk_base域控制位。
  reg ad1_count_enable_meta = 1'b0;
  reg ad1_count_enable = 1'b0;
  reg ad1_count_clear_meta = 1'b0;
  reg ad1_count_clear = 1'b0;
  reg ad2_count_enable_meta = 1'b0;
  reg ad2_count_enable = 1'b0;
  reg ad2_count_clear_meta = 1'b0;
  reg ad2_count_clear = 1'b0;

  always @(posedge AD1_INPUT_CLK) begin
    ad1_count_enable_meta <= control_data[9];
    ad1_count_enable <= ad1_count_enable_meta;
    ad1_count_clear_meta <= control_data[8];
    ad1_count_clear <= ad1_count_clear_meta;
  end

  always @(posedge AD2_INPUT_CLK) begin
    ad2_count_enable_meta <= control_data[11];
    ad2_count_enable <= ad2_count_enable_meta;
    ad2_count_clear_meta <= control_data[10];
    ad2_count_clear <= ad2_count_clear_meta;
  end

  // ADC采样时钟由本时钟域输出到AD芯片。第一级在时钟送达板外前保留上一
  // 个稳定转换字，第二级隔离多位并行总线；FIFO只接第二级，避免码型跨越
  // 0x800时把新旧数据位混合。采样沿脉冲仍只作为FIFO写使能。
  always @(posedge clk_base) begin
    ad1_input_pipe1 <= AD1_INPUT;
    ad1_input_pipe2 <= ad1_input_pipe1;
    ad2_input_pipe1 <= AD2_INPUT;
    ad2_input_pipe2 <= ad2_input_pipe1;
    sync_mode_d <= control_data[1];
    ad1_sampling_clock_d <= ad1_sampling_clock;
    ad2_sampling_clock_d <= ad2_sampling_clock;
    ad1_sample_strobe <= ad1_sampling_clock & ~ad1_sampling_clock_d;
    ad2_sample_strobe <= ad2_sampling_clock & ~ad2_sampling_clock_d;
  end

  assign sync_phase_load = control_data[1] & ~sync_mode_d;

  // 同步采集必须先记录AD1上升沿，再接受随后到来的AD2上升沿，确保两个
  // FIFO的第n个样本始终相差半个采样周期，而不受写使能落在哪个半周期影响。
  always @(posedge clk_base) begin
    if (!control_data[1] || !control_data[4] || !control_data[6])
      sync_capture_started <= 1'b0;
    else if (ad1_sample_strobe)
      sync_capture_started <= 1'b1;
  end

  assign ad2_capture_strobe = control_data[1]
      ? (ad2_sample_strobe & sync_capture_started) : ad2_sample_strobe;

  // 地址0 bit0为G题16384点长采集；Bit12/13仍选择原128点短采集。
  always @(posedge clk_base) begin
    if (!control_data[4]) begin
      ad1_capture_count <= 14'd0;
      ad1_capture_done <= 1'b0;
    end else if (ad1_sample_strobe && !ad1_capture_done) begin
      if ((control_data[12] && ad1_capture_count == 14'd127) ||
          (!control_data[12] && !ad1_long_capture &&
           ad1_capture_count == 14'd1023) ||
          (!control_data[12] && ad1_long_capture &&
           ad1_capture_count == 14'd16383))
        ad1_capture_done <= 1'b1;
      else
        ad1_capture_count <= ad1_capture_count + 1'b1;
    end

    if (!control_data[6]) begin
      ad2_capture_count <= 10'd0;
      ad2_capture_done <= 1'b0;
    end else if (ad2_capture_strobe && !ad2_capture_done) begin
      if ((control_data[13] && ad2_capture_count == 10'd127) ||
          (!control_data[13] && ad2_capture_count == 10'd1023))
        ad2_capture_done <= 1'b1;
      else
        ad2_capture_count <= ad2_capture_count + 1'b1;
    end
  end

  // DA数据在da_data_clock下降沿更新，采样时钟相对同频上升沿延后0.5ns。
  assign DA1_OUTCLK = da_capture_clock;
  assign DA2_OUTCLK = da_capture_clock;
  assign AD1_OUTCLK = ad1_sampling_clock;
  assign AD2_OUTCLK = ad2_sampling_clock;
  assign ad1_sampling_clock = ad1_sampling_clock_nco;
  // 两路始终直连各自NCO，避免组合时钟复用造成AD2跳码；相位模式通过
  // 同步装载0度/180度初相，使AD2上升沿对应AD1下降沿。
  assign ad2_sampling_clock = ad2_sampling_clock_nco;

  // FMC_CONTROL 已在 clk_base 域保存全部 16 个写寄存器，直接使用即可。
  assign control_data            = read_data_1;
  assign g_control_data          = read_data_0;
  assign ad1_long_capture        = g_control_data[0];
  assign da1_frequency_word_high = read_data_2;
  assign da1_frequency_word_low  = read_data_3;
  assign da2_frequency_word_high = read_data_4;
  assign da2_frequency_word_low  = read_data_5;
  assign ad1_sample_word_high    = read_data_6;
  assign ad1_sample_word_low     = read_data_7;
  assign ad2_sample_word_high    = read_data_8;
  assign ad2_sample_word_low     = read_data_9;
  assign da1_wave_select         = read_data_12[7:0];
  assign da2_wave_select         = read_data_12[15:8];
  assign da1_amplitude           = read_data_14[11:0];
  assign da2_amplitude           = read_data_15[11:0];
  // 0x0100~0x017F映射为128点自定义波形写窗口，两路RAM同步写入。
  assign custom_wave_write_enable = fmc_write_enable &&
                                    (address[15:7] == 9'h002);

  // FMC_CONTROL 已完成地址译码，读回数据直接组合映射，避免重复译码产生锁存器。
  assign ad1_fifo_data_read = {4'b0000,
      ad1_fifo_data[0], ad1_fifo_data[1], ad1_fifo_data[2], ad1_fifo_data[3],
      ad1_fifo_data[4], ad1_fifo_data[5], ad1_fifo_data[6], ad1_fifo_data[7],
      ad1_fifo_data[8], ad1_fifo_data[9], ad1_fifo_data[10], ad1_fifo_data[11]};
  assign ad2_fifo_data_read = {4'b0000,
      ad2_fifo_data[0], ad2_fifo_data[1], ad2_fifo_data[2], ad2_fifo_data[3],
      ad2_fifo_data[4], ad2_fifo_data[5], ad2_fifo_data[6], ad2_fifo_data[7],
      ad2_fifo_data[8], ad2_fifo_data[9], ad2_fifo_data[10], ad2_fifo_data[11]};
  assign ad1_fifo_flag_read = (ad1_capture_done || ad1_fifo_full) ? 16'h0001 : 16'h0000;
  assign ad2_fifo_flag_read = (ad2_capture_done || ad2_fifo_full) ? 16'h0001 : 16'h0000;

  // AD1 FIFO使用首字直通；每次FMC读事务结束后再推进读指针，
  // 保证STM32采样总线时FIFO输出不会同时发生多位翻转。
  assign ad1_fifo_read_active = fmc_read_enable && (address == 16'h0006);
  assign ad1_fifo_pop = ad1_fifo_read_sync_d && !ad1_fifo_read_sync &&
                        control_data[5];

  always @(posedge clk_base) begin
    ad1_fifo_read_meta <= ad1_fifo_read_active;
    ad1_fifo_read_sync <= ad1_fifo_read_meta;
    ad1_fifo_read_sync_d <= ad1_fifo_read_sync;
  end

  assign base1_frequency_high = base1_frequency_count[31:16];
  assign base1_frequency_low  = base1_frequency_count[15:0];
  assign base2_frequency_high = base2_frequency_count[31:16];
  assign base2_frequency_low  = base2_frequency_count[15:0];
  assign ad1_frequency_high   = ad1_frequency_count[31:16];
  assign ad1_frequency_low    = ad1_frequency_count[15:0];
  assign ad2_frequency_high   = ad2_frequency_count[31:16];
  assign ad2_frequency_low    = ad2_frequency_count[15:0];

  MYPLL pll (
      .inclk0(CLK),
      .c0    (clk_base),
      .c1    (da_data_clock),
      .c2    (da_capture_clock)
  );

  // STM32先停止DA再更新多位配置；两级同步后才重新启动DDS。
  always @(posedge da_data_clock) begin
    da_enable_meta <= control_data[0];
    da_enable_sync <= da_enable_meta;
    if (!da_enable_sync) begin
      da1_wave_select_da <= da1_wave_select;
      da2_wave_select_da <= da2_wave_select;
    end
  end

  test control_loopback (
      .old_data(control_data),
      .new_data(control_readback)
  );

  FMC_CONTROL fmc (
      .clk            (clk_base),
      .rst            (RST),
      .fpga_nl_nadv   (FPGA_NL_NADV),
      .fpga_cs_ne1    (FPGA_CS_NEL),
      .fpga_wr_nwe    (FPGA_WR_NWE),
      .fpga_rd_noe    (FPGA_RD_NOE),
      .fpga_db        (FPGA_DB),
      .write_data_0_  ({15'd0, ad1_long_capture}),
      .write_data_1_  (control_readback),
      .write_data_2_  (base1_frequency_high),
      .write_data_3_  (base1_frequency_low),
      .write_data_4_  (base2_frequency_high),
      .write_data_5_  (base2_frequency_low),
      .write_data_6_  (ad1_fifo_data_read),
      .write_data_7_  (ad1_fifo_flag_read),
      .write_data_8_  (ad2_fifo_data_read),
      .write_data_9_  (ad2_fifo_flag_read),
      .write_data_10_ (ad1_frequency_high),
      .write_data_11_ (ad1_frequency_low),
      .write_data_12_ (ad2_frequency_high),
      .write_data_13_ (ad2_frequency_low),
      .write_data_14_ (16'd0),
      .write_data_15_ (16'd0),
      .read_data_0_   (read_data_0),
      .read_data_1_   (read_data_1),
      .read_data_2_   (read_data_2),
      .read_data_3_   (read_data_3),
      .read_data_4_   (read_data_4),
      .read_data_5_   (read_data_5),
      .read_data_6_   (read_data_6),
      .read_data_7_   (read_data_7),
      .read_data_8_   (read_data_8),
      .read_data_9_   (read_data_9),
      .read_data_10_  (read_data_10),
      .read_data_11_  (read_data_11),
      .read_data_12_  (read_data_12),
      .read_data_13_  (read_data_13),
      .read_data_14_  (read_data_14),
      .read_data_15_  (read_data_15),
      .addr           (address),
      .fmc_wr_en      (fmc_write_enable),
      .fmc_rd_en      (fmc_read_enable)
  );

  DA_PARAMETER_CTRL da_control (
      .CLK_DA      (da_data_clock),
      .EN          (da_enable_sync),
      .FREQAH_W    (da1_frequency_word_high),
      .FREQAL_W    (da1_frequency_word_low),
      .FREQBH_W    (da2_frequency_word_high),
      .FREQBL_W    (da2_frequency_word_low),
      .PHASEA_IN   (read_data_10),
      .PHASEB_IN   (read_data_11),
      .COUT_A_FINAL(da1_rom_address),
      .COUT_B_FINAL(da2_rom_address)
  );

  DA_WAVEFORM da1_waveform (
      .CLK       (da_data_clock),
      .WAVE_SEL  (da1_wave_select_da),
      .PHASE_ADDR(da1_rom_address),
      .CUSTOM_DATA(da1_custom_wave_data),
      .WAVE_OUT  (da1_wave_data)
  );

  DA_WAVEFORM da2_waveform (
      .CLK       (da_data_clock),
      .WAVE_SEL  (da2_wave_select_da),
      .PHASE_ADDR(da2_rom_address),
      .CUSTOM_DATA(da2_custom_wave_data),
      .WAVE_OUT  (da2_wave_data)
  );

  DA_CUSTOM_WAVEFORM_RAM da1_custom_wave_ram (
      .WRITE_CLK (clk_base),
      .WRITE_EN  (custom_wave_write_enable),
      .WRITE_ADDR(address[6:0]),
      .WRITE_DATA(FPGA_DB[13:0]),
      .READ_CLK  (da_data_clock),
      .READ_ADDR (da1_rom_address),
      .READ_DATA (da1_custom_wave_data)
  );

  DA_CUSTOM_WAVEFORM_RAM da2_custom_wave_ram (
      .WRITE_CLK (clk_base),
      .WRITE_EN  (custom_wave_write_enable),
      .WRITE_ADDR(address[6:0]),
      .WRITE_DATA(FPGA_DB[13:0]),
      .READ_CLK  (da_data_clock),
      .READ_ADDR (da2_rom_address),
      .READ_DATA (da2_custom_wave_data)
  );

  VOLTAGE_SCALER_CLOCKED da1_voltage_scaler (
      .clk       (da_data_clock),
      .enable    (da_enable_sync),
      .rom_data  (da1_wave_data),
      .voltage_mv(da1_amplitude),
      .scaled_data(DA1_OUT)
  );

  VOLTAGE_SCALER_CLOCKED da2_voltage_scaler (
      .clk       (da_data_clock),
      .enable    (da_enable_sync),
      .rom_data  (da2_wave_data),
      .voltage_mv(da2_amplitude),
      .scaled_data(DA2_OUT)
  );

  FREQ_DEV ad1_sampling_frequency (
      .CLK     (clk_base),
      .EN      (control_data[2]),
      .PHASE_LOAD(sync_phase_load),
      .PHASE_INIT(32'h00000000),
      .FREQH_W (ad1_sample_word_high),
      .FREQL_W (ad1_sample_word_low),
      .FREQ_OUT(ad1_sampling_clock_nco)
  );

  FREQ_DEV ad2_sampling_frequency (
      .CLK     (clk_base),
      .EN      (control_data[3]),
      .PHASE_LOAD(sync_phase_load),
      .PHASE_INIT(32'h80000000),
      .FREQH_W (ad2_sample_word_high),
      .FREQL_W (ad2_sample_word_low),
      .FREQ_OUT(ad2_sampling_clock_nco)
  );

  G_AD_FIFO ad1_fifo (
      .data  (ad1_input_pipe2),
      .aclr  (control_data[14]),
      .wrreq (control_data[4] & ad1_sample_strobe & ~ad1_capture_done),
      .wrclk (clk_base),
      .rdreq (ad1_fifo_pop),
      .rdclk (clk_base),
      .wrfull(ad1_fifo_full),
      .q     (ad1_fifo_data)
  );

  TYFIFO ad2_fifo (
      .data  (ad2_input_pipe2),
      .aclr  (control_data[15]),
      .wrreq (control_data[6] & ad2_capture_strobe & ~ad2_capture_done),
      .wrclk (clk_base),
      .rdreq (control_data[7]),
      .rdclk (fmc_read_enable),
      .wrfull(ad2_fifo_full),
      .q     (ad2_fifo_data)
  );

  CNT32 ad1_frequency_counter (
      .CLR      (ad1_count_clear),
      .CLRBASE  (control_data[8]),
      .CLK      (AD1_INPUT_CLK),
      .CLKBASE  (clk_base),
      .CLKEN    (ad1_count_enable),
      .CLKBASEEN(control_data[9]),
      .Q        (ad1_frequency_count),
      .QBASE    (base1_frequency_count)
  );

  CNT32 ad2_frequency_counter (
      .CLR      (ad2_count_clear),
      .CLRBASE  (control_data[10]),
      .CLK      (AD2_INPUT_CLK),
      .CLKBASE  (clk_base),
      .CLKEN    (ad2_count_enable),
      .CLKBASEEN(control_data[11]),
      .Q        (ad2_frequency_count),
      .QBASE    (base2_frequency_count)
  );

endmodule
