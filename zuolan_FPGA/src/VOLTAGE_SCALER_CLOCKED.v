// 作者：左岚
// 125MHz流水线幅度缩放器。
// 使用Q24倒数近似除以3080，最大幅值误差小于0.01%。
module VOLTAGE_SCALER_CLOCKED (
    input             clk,
    input             enable,
    input      [13:0] rom_data,
    input      [11:0] voltage_mv,
    output reg [13:0] scaled_data
);

  localparam [13:0] HALF_ROM_MAX = 14'd8191;
  localparam [11:0] MAX_VOLTAGE_MV = 12'd3080;
  localparam [12:0] RECIPROCAL_Q24 = 13'd5447;

  reg [11:0] voltage_meta = 12'd0;
  reg [11:0] voltage_sync = 12'd0;
  reg [13:0] magnitude_pipe1 = 14'd0;
  reg        positive_pipe1 = 1'b0;
  reg [24:0] product_pipe2 = 25'd0;
  reg        positive_pipe2 = 1'b0;
  reg [37:0] reciprocal_product_pipe3 = 38'd0;
  reg        positive_pipe3 = 1'b0;
  reg [13:0] scaled_data_pipe4 = HALF_ROM_MAX;
  reg        enable_negedge = 1'b0;

  wire [13:0] scaled_magnitude = reciprocal_product_pipe3[37:24];

  always @(posedge clk) begin
    voltage_meta <= (voltage_mv > MAX_VOLTAGE_MV) ? MAX_VOLTAGE_MV : voltage_mv;
    voltage_sync <= voltage_meta;

    positive_pipe1 <= (rom_data >= HALF_ROM_MAX);
    magnitude_pipe1 <= (rom_data >= HALF_ROM_MAX)
        ? (rom_data - HALF_ROM_MAX)
        : (HALF_ROM_MAX - rom_data);

    product_pipe2 <= magnitude_pipe1 * voltage_sync;
    positive_pipe2 <= positive_pipe1;

    reciprocal_product_pipe3 <= product_pipe2 * RECIPROCAL_Q24;
    positive_pipe3 <= positive_pipe2;

    scaled_data_pipe4 <= positive_pipe3
        ? (HALF_ROM_MAX + scaled_magnitude)
        : (HALF_ROM_MAX - scaled_magnitude);
  end

  // 数据在125MHz时钟下降沿更新，外部AD9764在同一时钟上升沿采样。
  // 分级运算与最终输出错开半个周期，避免数据翻转贴近采样边沿。
  always @(negedge clk) begin
    enable_negedge <= enable;
    if (!enable_negedge)
      scaled_data <= HALF_ROM_MAX;
    else
      scaled_data <= scaled_data_pipe4;
  end

endmodule
