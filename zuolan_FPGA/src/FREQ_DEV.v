// 作者：左岚
// 32位方波NCO：Fout = Fclk * {FREQH_W, FREQL_W} / 2^32。
// PHASE_LOAD优先于EN，用于双通道同步装载固定初相。

module FREQ_DEV (
    input             CLK,
    input             EN,
    input             PHASE_LOAD,
    input      [31:0] PHASE_INIT,
    input      [15:0] FREQH_W,
    input      [15:0] FREQL_W,
    output reg        FREQ_OUT
);

  reg [31:0] FREQ_WORD;
  reg [31:0] ACC = 32'd0;

  always @(posedge CLK) begin
    if (PHASE_LOAD) begin
      ACC <= PHASE_INIT;
      FREQ_OUT <= PHASE_INIT[31];
    end else begin
      if (EN) begin
        ACC <= ACC + FREQ_WORD;
      end
      FREQ_OUT <= ACC[31];
    end
  end

  // FMC寄存器在EN关闭时更新；此处再同步到NCO域。
  always @(posedge CLK) begin
    FREQ_WORD <= {FREQH_W, FREQL_W};
  end

endmodule
