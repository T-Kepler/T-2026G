// 作者：左岚
// 单通道128点波形ROM选择器，ROM输出后再经过两级流水。
module DA_WAVEFORM #(
    parameter SINE_WAVE     = 8'd0,
    parameter SQUARE_WAVE   = 8'd1,
    parameter TRIANGLE_WAVE = 8'd2,
    parameter SAWTOOTH_WAVE = 8'd3,
    parameter CUSTOM_WAVE   = 8'd4
) (
    input         CLK,
    input  [ 7:0] WAVE_SEL,
    input  [ 6:0] PHASE_ADDR,
    input  [13:0] CUSTOM_DATA,
    output [13:0] WAVE_OUT
);

  wire [13:0] sin_data;
  wire [13:0] tri_data;
  wire [13:0] squ_data;
  wire [13:0] swt_data;

  reg  [13:0] selected_wave_data;
  reg  [13:0] wave_data_pipe1;
  reg  [13:0] wave_data_pipe2;

  // 四个同步ROM共享相位地址。
  sin_rom sin_rom_inst (
      .address(PHASE_ADDR),
      .clock  (CLK),
      .q      (sin_data)
  );

  sqaure_rom sqaure_rom_inst (
      .address(PHASE_ADDR),
      .clock  (CLK),
      .q      (squ_data)
  );

  triangle_rom triangle_rom_inst (
      .address(PHASE_ADDR),
      .clock  (CLK),
      .q      (tri_data)
  );

  sawtooth_rom sawtooth_rom_inst (
      .address(PHASE_ADDR),
      .clock  (CLK),
      .q      (swt_data)
  );

  always @(*) begin
    case (WAVE_SEL)
      SQUARE_WAVE:   selected_wave_data = squ_data;
      TRIANGLE_WAVE: selected_wave_data = tri_data;
      SAWTOOTH_WAVE: selected_wave_data = swt_data;
      CUSTOM_WAVE:   selected_wave_data = CUSTOM_DATA;
      SINE_WAVE:     selected_wave_data = sin_data;
      default:       selected_wave_data = sin_data;
    endcase
  end

  always @(posedge CLK) begin
    wave_data_pipe1 <= selected_wave_data;
    wave_data_pipe2 <= wave_data_pipe1;
  end

  assign WAVE_OUT = wave_data_pipe2;

endmodule

// STM32写、DA读的128点双时钟波形RAM。两个实例共用写口数据，读地址独立。
module DA_CUSTOM_WAVEFORM_RAM (
    input         WRITE_CLK,
    input         WRITE_EN,
    input  [ 6:0] WRITE_ADDR,
    input  [13:0] WRITE_DATA,
    input         READ_CLK,
    input  [ 6:0] READ_ADDR,
    output reg [13:0] READ_DATA
);

  reg [13:0] memory [0:127];

  always @(posedge WRITE_CLK) begin
    if (WRITE_EN) memory[WRITE_ADDR] <= WRITE_DATA;
  end

  always @(posedge READ_CLK) begin
    READ_DATA <= memory[READ_ADDR];
  end

endmodule
