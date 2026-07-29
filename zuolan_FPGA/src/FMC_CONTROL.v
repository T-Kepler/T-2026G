// 作者：左岚
// 16位异步FMC从接口：锁存复用地址，保存16个写寄存器并返回16个读通道。
// 端口名称沿用原工程：write_data_*表示FPGA→STM32，read_data_*表示STM32→FPGA。

module FMC_CONTROL (
    input clk,
    input rst,  // 低有效

    input           fpga_nl_nadv,  // 地址阶段低有效
    input fpga_cs_ne1,             // 片选，低有效
    input fpga_wr_nwe,             // 写使能，低有效
    input fpga_rd_noe,             // 读使能，低有效
    inout [15:0] fpga_db,

    // FPGA -> STM32
    input [15:0] write_data_0_,
    input [15:0] write_data_1_,
    input [15:0] write_data_2_,
    input [15:0] write_data_3_,
    input [15:0] write_data_4_,
    input [15:0] write_data_5_,
    input [15:0] write_data_6_,
    input [15:0] write_data_7_,
    input [15:0] write_data_8_,
    input [15:0] write_data_9_,
    input [15:0] write_data_10_,
    input [15:0] write_data_11_,
    input [15:0] write_data_12_,
    input [15:0] write_data_13_,
    input [15:0] write_data_14_,
    input [15:0] write_data_15_,

    // STM32 -> FPGA
    output [15:0] read_data_0_,
    output [15:0] read_data_1_,
    output [15:0] read_data_2_,
    output [15:0] read_data_3_,
    output [15:0] read_data_4_,
    output [15:0] read_data_5_,
    output [15:0] read_data_6_,
    output [15:0] read_data_7_,
    output [15:0] read_data_8_,
    output [15:0] read_data_9_,
    output [15:0] read_data_10_,
    output [15:0] read_data_11_,
    output [15:0] read_data_12_,
    output [15:0] read_data_13_,
    output [15:0] read_data_14_,
    output [15:0] read_data_15_,

    output [15:0] addr,
    output        fmc_wr_en,
    output        fmc_rd_en
);

  // STM32写寄存器。
  reg [15:0] read_data_0__reg;
  reg [15:0] read_data_1__reg;
  reg [15:0] read_data_2__reg;
  reg [15:0] read_data_3__reg;
  reg [15:0] read_data_4__reg;
  reg [15:0] read_data_5__reg;
  reg [15:0] read_data_6__reg;
  reg [15:0] read_data_7__reg;
  reg [15:0] read_data_8__reg;
  reg [15:0] read_data_9__reg;
  reg [15:0] read_data_10__reg;
  reg [15:0] read_data_11__reg;
  reg [15:0] read_data_12__reg;
  reg [15:0] read_data_13__reg;
  reg [15:0] read_data_14__reg;
  reg [15:0] read_data_15__reg;

  // 非FIFO读通道缓存。
  reg [15:0] rd_data_reg;
  wire fifo_read_address;
  wire [15:0] fifo_read_data;

  assign read_data_0_ = read_data_0__reg;
  assign read_data_1_ = read_data_1__reg;
  assign read_data_2_ = read_data_2__reg;
  assign read_data_3_ = read_data_3__reg;
  assign read_data_4_ = read_data_4__reg;
  assign read_data_5_ = read_data_5__reg;
  assign read_data_6_ = read_data_6__reg;
  assign read_data_7_ = read_data_7__reg;
  assign read_data_8_ = read_data_8__reg;
  assign read_data_9_ = read_data_9__reg;
  assign read_data_10_ = read_data_10__reg;
  assign read_data_11_ = read_data_11__reg;
  assign read_data_12_ = read_data_12__reg;
  assign read_data_13_ = read_data_13__reg;
  assign read_data_14_ = read_data_14__reg;
  assign read_data_15_ = read_data_15__reg;

  // 数据阶段读写使能。
  assign fmc_wr_en = ((!fpga_cs_ne1) & (!fpga_wr_nwe) & fpga_nl_nadv);

  assign fmc_rd_en = ((!fpga_cs_ne1) & (!fpga_rd_noe) & fpga_nl_nadv);

  // FIFO数据由顶层在读事务结束后推进，当前事务内保持稳定并直接返回。
  assign fifo_read_address = (addr == 16'h0006) || (addr == 16'h0008);
  assign fifo_read_data = (addr == 16'h0006) ? write_data_6_ : write_data_8_;
  assign fpga_db = fmc_rd_en ? (fifo_read_address ? fifo_read_data : rd_data_reg)
                              : 16'hzzzz;

  // 复用总线地址阶段锁存；该自反馈写法综合为锁存器，板级FMC时序必须满足要求。
  assign addr = ((fpga_nl_nadv == 1'b0) & (fpga_cs_ne1 == 1'b0)) ? fpga_db : addr;

  always @(posedge clk or negedge rst) begin
    if (!rst) begin
      read_data_0__reg  <= 16'd0;
      read_data_1__reg  <= 16'd0;
      read_data_2__reg  <= 16'd0;
      read_data_3__reg  <= 16'd0;
      read_data_4__reg  <= 16'd0;
      read_data_5__reg  <= 16'd0;
      read_data_6__reg  <= 16'd0;
      read_data_7__reg  <= 16'd0;
      read_data_8__reg  <= 16'd0;
      read_data_9__reg  <= 16'd0;
      read_data_10__reg <= 16'd0;
      read_data_11__reg <= 16'd0;
      read_data_12__reg <= 16'd0;
      read_data_13__reg <= 16'd0;
      read_data_14__reg <= 16'd0;
      read_data_15__reg <= 16'd0;
      rd_data_reg       <= 16'd0;
    end
    else if (fmc_wr_en) begin
      case (addr)
        16'h0000: read_data_0__reg <= fpga_db;
        16'h0001: read_data_1__reg <= fpga_db;
        16'h0002: read_data_2__reg <= fpga_db;
        16'h0003: read_data_3__reg <= fpga_db;
        16'h0004: read_data_4__reg <= fpga_db;
        16'h0005: read_data_5__reg <= fpga_db;
        16'h0006: read_data_6__reg <= fpga_db;
        16'h0007: read_data_7__reg <= fpga_db;
        16'h0008: read_data_8__reg <= fpga_db;
        16'h0009: read_data_9__reg <= fpga_db;
        16'h000A: read_data_10__reg <= fpga_db;
        16'h000B: read_data_11__reg <= fpga_db;
        16'h000C: read_data_12__reg <= fpga_db;
        16'h000D: read_data_13__reg <= fpga_db;
        16'h000E: read_data_14__reg <= fpga_db;
        16'h000F: read_data_15__reg <= fpga_db;
        default:  ;
      endcase
    end
    else if (fmc_rd_en) begin
      case (addr)
        16'h0000: rd_data_reg <= write_data_0_;
        16'h0001: rd_data_reg <= write_data_1_;
        16'h0002: rd_data_reg <= write_data_2_;
        16'h0003: rd_data_reg <= write_data_3_;
        16'h0004: rd_data_reg <= write_data_4_;
        16'h0005: rd_data_reg <= write_data_5_;
        16'h0006: ;
        16'h0007: rd_data_reg <= write_data_7_;
        16'h0008: ;
        16'h0009: rd_data_reg <= write_data_9_;
        16'h000A: rd_data_reg <= write_data_10_;
        16'h000B: rd_data_reg <= write_data_11_;
        16'h000C: rd_data_reg <= write_data_12_;
        16'h000D: rd_data_reg <= write_data_13_;
        16'h000E: rd_data_reg <= write_data_14_;
        16'h000F: rd_data_reg <= write_data_15_;
        default: rd_data_reg <= 16'd0;
      endcase
    end
  end

endmodule
