// 作者：左岚
// 固定 125MHz 采样时钟下的双通道 DDS 相位累加器。
// 32 位频率字决定输出频率，累加器高 7 位寻址 128 点波形 ROM。
module DA_PARAMETER_CTRL #(
    parameter PHASE_WIDTH = 7
) (
    input                         CLK_DA,
    input                         EN,
    input  [15:0]                 FREQAH_W,
    input  [15:0]                 FREQAL_W,
    input  [15:0]                 FREQBH_W,
    input  [15:0]                 FREQBL_W,
    input  [15:0]                 PHASEA_IN,
    input  [15:0]                 PHASEB_IN,
    output [PHASE_WIDTH-1:0]       COUT_A_FINAL,
    output [PHASE_WIDTH-1:0]       COUT_B_FINAL
);

  reg  [31:0] frequency_word_a = 32'd0;
  reg  [31:0] frequency_word_b = 32'd0;
  reg  [PHASE_WIDTH-1:0] phase_offset_a = {PHASE_WIDTH{1'b0}};
  reg  [PHASE_WIDTH-1:0] phase_offset_b = {PHASE_WIDTH{1'b0}};
  reg  [31:0] accumulator_a = 32'd0;
  reg  [31:0] accumulator_b = 32'd0;

  always @(posedge CLK_DA) begin
    if (EN) begin
      accumulator_a <= accumulator_a + frequency_word_a;
      accumulator_b <= accumulator_b + frequency_word_b;
    end else begin
      frequency_word_a <= {FREQAH_W, FREQAL_W};
      frequency_word_b <= {FREQBH_W, FREQBL_W};
      phase_offset_a <= PHASEA_IN[PHASE_WIDTH-1:0];
      phase_offset_b <= PHASEB_IN[PHASE_WIDTH-1:0];
      accumulator_a <= 32'd0;
      accumulator_b <= 32'd0;
    end
  end

  assign COUT_A_FINAL = accumulator_a[31 -: PHASE_WIDTH] + phase_offset_a;
  assign COUT_B_FINAL = accumulator_b[31 -: PHASE_WIDTH] + phase_offset_b;

endmodule
