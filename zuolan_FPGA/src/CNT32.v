// 作者：左岚
// 两个独立时钟域的32位计数器，用于比较器脉冲和基准时钟门控计数。

module CNT32 (
    input         CLR,        // 被测信号计数器同步清零，低有效
    input         CLRBASE,    // 基准计数器同步清零，低电平有效
    input         CLK,        // 被测比较器脉冲
    input         CLKBASE,    // 基准时钟
    input         CLKEN,      // 被测信号计数使能
    input         CLKBASEEN,  // 基准计数使能
    output [31:0] Q,
    output [31:0] QBASE
);

  reg [31:0] Q1 = 32'd0;
  reg [31:0] Q1BASE = 32'd0;

  always @(posedge CLK) begin
    if (!CLR) begin
      Q1 <= 32'b0;
    end
    else if (CLKEN) begin
      Q1 <= Q1 + 1;
    end
  end

  assign Q = Q1;

  always @(posedge CLKBASE) begin
    if (!CLRBASE) begin
      Q1BASE <= 32'b0;
    end
    else if (CLKBASEEN) begin
      Q1BASE <= Q1BASE + 1;
    end
  end

  assign QBASE = Q1BASE;

endmodule
