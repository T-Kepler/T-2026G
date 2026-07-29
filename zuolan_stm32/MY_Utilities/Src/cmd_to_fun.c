/**
 * @file cmd_to_fun.c
 * @brief FPGA控制命令到底层函数的映射实现
 * @details
 * 该文件实现了一系列底层函数，用于将高级功能命令转换为对FPGA硬件控制寄存器(`CTRL_DATA`)
 * 的具体位操作。它提供了一个清晰的硬件抽象层，封装了对DA输出、AD频率测量、AD数据采集等
 * 模块的启动、停止和配置。所有函数通过置位或清零`CTRL_DATA`寄存器中的特定控制位来与FPGA交互。
 *
 * @author 左岚
 * @date 2025-07-18
 */
#include "cmd_to_fun.h"

/**
 * @brief 初始化FPGA主控制寄存器
 * @details 将FPGA的全局控制寄存器 `CTRL_DATA` 的所有位清零，
 * 使其处于一个已知的默认和安全状态。
 */
void CTRL_INIT(void)
{
    G_CAPTURE_CTRL = 0U;
    CTRL_DATA = 0; // 将控制寄存器所有位清零
}

// =================================================================================
// DA (数模转换) 控制系列
// =================================================================================

/**
 * @brief 启动FPGA的DA波形发生器
 * @details 通过对 `CTRL_DATA` 寄存器执行按位“或”操作，将 `DA_FREQ_EN` 控制位置为1。
 * 这会使能FPGA内部的DA模块，使其开始生成并输出波形，同时不影响其他控制位的状态。
 */
void DA_FPGA_START()
{
    CTRL_DATA = CTRL_DATA | DA_FREQ_EN; // 置位DA使能位，启动DA输出
}

/**
 * @brief 停止FPGA的DA波形发生器
 * @details 通过对 `CTRL_DATA` 寄存器执行按位“与非”操作，将 `DA_FREQ_EN` 控制位清零。
 * 这会禁用FPGA内部的DA模块，使其停止输出波形，同时保持其他控制位不变。
 */
void DA_FPGA_STOP()
{
    CTRL_DATA = CTRL_DATA & (~DA_FREQ_EN); // 清零DA使能位，停止DA输出
}

// =================================================================================
// AD (模数转换) 频率测量系列
// =================================================================================

/**
 * @brief 使能AD测频计数器清零（复位）
 * @details 此函数通过将指定的 `ADx_FREQ_CLR` 控制位清零（置0）来触发FPGA内部测频计数器的复位操作。
 * 这是一个低电平有效的清零信号，即该位为0时，计数器被持续清零。
 * @param ch 通道号 (1: AD1, 2: AD2)
 */
void AD_FREQ_CLR_ENABLE(int ch)
{
    switch (ch) {
    case 1:
        CTRL_DATA = CTRL_DATA & (~AD1_FREQ_CLR); // 将AD1清零位置0，使能清零功能
        break;
    case 2:
        CTRL_DATA = CTRL_DATA & (~AD2_FREQ_CLR); // 将AD2清零位置0，使能清零功能
        break;
    }
}

/**
 * @brief 禁能AD测频计数器清零（释放复位）
 * @details 此函数通过将指定的 `ADx_FREQ_CLR` 控制位置位（置1），使清零信号变为无效状态。
 * 这会让测频计数器退出复位状态，可以开始正常地进行计数。
 * @param ch 通道号 (1: AD1, 2: AD2)
 */
void AD_FREQ_CLR_DISABLE(int ch)
{
    switch (ch) {
    case 1:
        CTRL_DATA = CTRL_DATA | AD1_FREQ_CLR; // 将AD1清零位置1，禁能清零功能
        break;
    case 2:
        CTRL_DATA = CTRL_DATA | AD2_FREQ_CLR; // 将AD2清零位置1，禁能清零功能
        break;
    }
}

/**
 * @brief 启动AD频率测量
 * @details 通过置位 `ADx_FREQ_START` 控制位来启动FPGA的频率计数过程。
 * 这是一个高电平有效的控制信号，即该位为1时，计数器工作。
 * @param ch 通道号 (1: 启动AD1测频, 2: 启动AD2测频)
 */
void AD_FREQ_START(int ch)
{
    switch (ch) {
    case 1:
        CTRL_DATA = CTRL_DATA | AD1_FREQ_START; // 置位AD1启动位
        break;
    case 2:
        CTRL_DATA = CTRL_DATA | AD2_FREQ_START; // 置位AD2启动位
        break;
    }
}

/**
 * @brief 停止AD频率测量
 * @details 通过清零 `ADx_FREQ_START` 控制位来停止FPGA的频率计数过程。
 * @param ch 通道号 (1: 停止AD1测频, 2: 停止AD2测频)
 */
void AD_FREQ_STOP(int ch)
{
    switch (ch) {
    case 1:
        CTRL_DATA = CTRL_DATA & (~AD1_FREQ_START); // 清零AD1启动位
        break;
    case 2:
        CTRL_DATA = CTRL_DATA & (~AD2_FREQ_START); // 清零AD2启动位
        break;
    }
}

// =================================================================================
// AD (模数转换) 数据采集系列
// =================================================================================

/**
 * @brief 使能AD采样时钟生成
 * @details 通过置位 `ADx_FREQ_EN` 控制位，使能FPGA为指定的AD通道生成采样时钟。
 * 这是启动AD数据采集并写入FIFO的前提条件。
 * @param ch 通道号 (1: AD1, 2: AD2, 3: AD1和AD2同时)
 */
void AD_FREQ_SET(int ch)
{
    switch (ch) {
    case 1:
        CTRL_DATA = CTRL_DATA | AD1_FREQ_EN; // 使能AD1采样时钟
        break;
    case 2:
        CTRL_DATA = CTRL_DATA | AD2_FREQ_EN; // 使能AD2采样时钟
        break;
    case 3:
        CTRL_DATA = CTRL_DATA | AD1_FREQ_EN | AD2_FREQ_EN; // 同时使能两个通道
        break;
    }
}

/**
 * @brief 使能AD数据写入FIFO
 * @details 通过置位 `ADx_FIFO_WR` 控制位，允许AD转换后的数据写入到FPGA内部的FIFO缓存中。
 * @param ch 通道号 (1: AD1, 2: AD2, 3: AD1和AD2同时)
 */
void AD_FIFO_WRITE_ENABLE(int ch)
{
    switch (ch) {
    case 1:
        CTRL_DATA = CTRL_DATA | AD1_FIFO_WR; // 允许AD1数据写入FIFO
        break;
    case 2:
        CTRL_DATA = CTRL_DATA | AD2_FIFO_WR; // 允许AD2数据写入FIFO
        break;
    case 3:
        CTRL_DATA = CTRL_DATA | AD1_FIFO_WR | AD2_FIFO_WR; // 同时允许两个通道写入
        break;
    }
}

/**
 * @brief 禁能AD数据写入FIFO
 * @details 通过清零 `ADx_FIFO_WR` 控制位，停止AD数据向FIFO缓存的写入。
 * @param ch 通道号 (1: AD1, 2: AD2)
 */
void AD_FIFO_WRITE_DISABLE(int ch)
{
    switch (ch) {
    case 1:
        CTRL_DATA = CTRL_DATA & (~AD1_FIFO_WR); // 禁止AD1数据写入FIFO
        break;
    case 2:
        CTRL_DATA = CTRL_DATA & (~AD2_FIFO_WR); // 禁止AD2数据写入FIFO
        break;
    }
}

/**
 * @brief 使能从FIFO读取AD数据
 * @details 通过置位 `ADx_FIFO_RD` 控制位，允许微控制器(MCU)从FPGA的FIFO缓存中读取数据。
 * @param ch 通道号 (1: AD1, 2: AD2)
 */
void AD_FIFO_READ_ENABLE(int ch)
{
    switch (ch) {
    case 1:
        CTRL_DATA = CTRL_DATA | AD1_FIFO_RD; // 允许从AD1的FIFO读取数据
        break;
    case 2:
        CTRL_DATA = CTRL_DATA | AD2_FIFO_RD; // 允许从AD2的FIFO读取数据
        break;
    }
}

/**
 * @brief 禁能从FIFO读取AD数据
 * @details 通过清零 `ADx_FIFO_RD` 控制位，禁止从FIFO读取数据。通常在数据读取操作完成后调用。
 * @param ch 通道号 (1: AD1, 2: AD2)
 */
void AD_FIFO_READ_DISABLE(int ch)
{
    switch (ch) {
    case 1:
        CTRL_DATA = CTRL_DATA & (~AD1_FIFO_RD); // 禁止从AD1的FIFO读取
        break;
    case 2:
        CTRL_DATA = CTRL_DATA & (~AD2_FIFO_RD); // 禁止从AD2的FIFO读取
        break;
    }
}
