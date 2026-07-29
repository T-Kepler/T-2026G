/**
 ******************************************************************************
 * @file    AD9833.c
 * @author  左岚、调库侠
 * @brief   AD9833波形发生器驱动程序的实现文件。
 ******************************************************************************
 */

#include "AD9833.h"

/******************************************************************************
 * 第一个AD9833设备的功能函数                                                  *
 ******************************************************************************/

/**
 * @brief  初始化第一个AD9833的GPIO引脚。
 * @note   此函数配置GPIOF上的SDATA, SCLK, FSYNC引脚为推挽输出模式。
 * 假设引脚定义 (AD9833_SDATA1_Pin等) 已在 main.h 中定义。
 * @param  无
 * @retval 无
 */
void AD9833_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 使能GPIOF端口时钟
    __HAL_RCC_GPIOF_CLK_ENABLE();

    // 配置GPIO引脚: SDATA1, SCLK1, FSYNC1
    GPIO_InitStruct.Pin = AD9833_SDATA1_Pin | AD9833_SCLK1_Pin | AD9833_FSYNC1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   // 推挽输出
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; // 高速模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;           // 无上下拉
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    // 设置SPI引脚的初始电平为高电平
    HAL_GPIO_WritePin(AD9833_SDATA1_GPIO_Port, AD9833_SDATA1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AD9833_SCLK1_GPIO_Port, AD9833_SCLK1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AD9833_FSYNC1_GPIO_Port, AD9833_FSYNC1_Pin, GPIO_PIN_SET);
}

/**
 * @brief  软件延时函数。
 * @note   这是一个非常短的延时，用于满足AD9833的SPI时序要求。
 * 其延时长度取决于MCU的时钟速度和编译器优化等级。
 * @param  无
 * @retval 无
 */
static void AD9833_Delay(void)
{
    uint16_t i;
    for (i = 0; i < 1; i++)
        ;
}

/**
 * @brief  向第一个AD9833写入16位数据。
 * @note   此函数通过软件模拟SPI协议发送数据。
 * @param  txdata: 待写入的16位数据。
 * @retval 无
 */
void AD9833_WriteData(uint16_t txdata)
{
    int i;

    // SPI通信开始前，确保SCLK和FSYNC为高电平
    HAL_GPIO_WritePin(AD9833_SCLK1_GPIO_Port, AD9833_SCLK1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AD9833_FSYNC1_GPIO_Port, AD9833_FSYNC1_Pin, GPIO_PIN_SET);

    // 拉低FSYNC，启动一次数据传输
    HAL_GPIO_WritePin(AD9833_FSYNC1_GPIO_Port, AD9833_FSYNC1_Pin, GPIO_PIN_RESET);

    // 循环发送16位数据，从最高位(MSB)开始
    for (i = 0; i < 16; i++)
    {
        // 检查当前最高位是1还是0
        if (txdata & 0x8000)
            HAL_GPIO_WritePin(AD9833_SDATA1_GPIO_Port, AD9833_SDATA1_Pin, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(AD9833_SDATA1_GPIO_Port, AD9833_SDATA1_Pin, GPIO_PIN_RESET);

        // 产生一个SCLK时钟脉冲：下降沿 -> 延时 -> 上升沿
        AD9833_Delay();
        HAL_GPIO_WritePin(AD9833_SCLK1_GPIO_Port, AD9833_SCLK1_Pin, GPIO_PIN_RESET);
        AD9833_Delay();
        HAL_GPIO_WritePin(AD9833_SCLK1_GPIO_Port, AD9833_SCLK1_Pin, GPIO_PIN_SET);

        // 将数据左移一位，准备发送下一位
        txdata <<= 1;
    }

    // 拉高FSYNC，结束本次数据传输
    HAL_GPIO_WritePin(AD9833_FSYNC1_GPIO_Port, AD9833_FSYNC1_Pin, GPIO_PIN_SET);
}

/**
 * @brief  设置第一个AD9833的频率寄存器。
 * @note   频率计算公式: Fout = (MCLK / 2^28) * FREQ_REG_VALUE
 * MCLK为外部晶振频率，这里假设为25MHz。
 * @param  reg:  目标频率寄存器 (AD9833_REG_FREQ0 或 AD9833_REG_FREQ1)。
 * @param  fout: 频率值 (单位: Hz)。
 * @retval 无
 */
void AD9833_SetFrequency(unsigned short reg, double fout)
{
    int frequence_LSB, frequence_MSB;
    double frequence_mid, frequence_DATA;
    long int frequence_hex;

    // 计算频率调谐字
    // frequence_mid = 2^28 / MCLK_in_MHz
    frequence_mid = 268435456.0 / 25.0; // 适用于25MHz晶振
    // 如果MCLK频率不是25MHz，请修改这里的数值

    frequence_DATA = fout;
    frequence_DATA = frequence_DATA / 1000000.0; // 将Hz转换为MHz以匹配frequence_mid的单位
    frequence_DATA = frequence_DATA * frequence_mid;
    frequence_hex = frequence_DATA; // 将32位浮点数转换为32位长整型

    // AD9833的频率寄存器是28位的，需要拆分成两个14位的数据包发送
    frequence_LSB = (int)frequence_hex;         // 取低16位
    frequence_LSB &= 0x3FFF;                    // 屏蔽高两位，保留低14位
    frequence_MSB = (int)(frequence_hex >> 14); // 右移14位，得到高14位
    frequence_MSB &= 0x3FFF;                    // 屏蔽高两位，保留高14位

    // 将寄存器地址与14位数据合并
    frequence_LSB |= reg;
    frequence_MSB |= reg;

    // 发送数据到AD9833
    // 首先发送控制字0x2100 (B28=1, RESET=1)，表示接下来将进行一次完整的28位频率写入，同时保持复位状态
    AD9833_WriteData(0x2100);
    // 写入频率数据的低14位
    AD9833_WriteData(frequence_LSB);
    // 写入频率数据的高14位
    AD9833_WriteData(frequence_MSB);
}

/**
 * @brief  设置第一个AD9833的相位寄存器。
 * @param  reg: 目标相位寄存器 (AD9833_REG_PHASE0 或 AD9833_REG_PHASE1)。
 * @param  val: 相位值 (0-4095, 对应0-2π)。
 * @retval 无
 */
void AD9833_SetPhase(unsigned short reg, unsigned short val)
{
    unsigned short phase = reg;
    phase |= (val & 0x0FFF); // 确保相位值在12位以内，并与寄存器地址合并
    AD9833_WriteData(phase);
}

/**
 * @brief  设置第一个AD9833的输出波形。
 * @param  WaveMode:  波形类型 (例如 AD9833_OUT_SINUS)。
 * @param  Freq_SFR:  选择的频率寄存器 (AD9833_FSEL0 或 AD9833_FSEL1)。
 * @param  Phase_SFR: 选择的相位寄存器 (AD9833_PSEL0 或 AD9833_PSEL1)。
 * @retval 无
 */
void AD9833_SetWave(unsigned int WaveMode, unsigned int Freq_SFR, unsigned int Phase_SFR)
{
    // 与官方驱动一致，保持B28以便后续完整写入28位频率字。
    unsigned int val = AD9833_B28 | WaveMode | Freq_SFR | Phase_SFR;
    // 写入控制寄存器，此时应清除RESET位(bit 8)，让芯片开始工作
    AD9833_WriteData(val & ~AD9833_RESET);
}

/**
 * @brief  完整配置第一个AD9833的输出参数。
 * @param  Freq_SFR:  目标频率寄存器。
 * @param  Freq:      频率值(Hz)。
 * @param  Phase_SFR: 目标相位寄存器。
 * @param  Phase:     相位值(0-4095)。
 * @param  WaveMode:  波形类型。
 * @retval 无
 */
void AD9833_Setup(unsigned int Freq_SFR, double Freq, unsigned int Phase_SFR, unsigned int Phase, unsigned int WaveMode)
{
    unsigned int Fsel, Psel;

    // 将AD9833置于复位状态
    AD9833_WriteData(AD9833_RESET);

    // 设置频率和相位值
    AD9833_SetFrequency(Freq_SFR, Freq);
    AD9833_SetPhase(Phase_SFR, Phase);

    // 根据传入的寄存器类型，选择对应的FSEL和PSEL控制位
    if (Freq_SFR == AD9833_REG_FREQ0)
    {
        Fsel = AD9833_FSEL0;
    }
    else
    {
        Fsel = AD9833_FSEL1;
    }

    if (Phase_SFR == AD9833_REG_PHASE0)
    {
        Psel = AD9833_PSEL0;
    }
    else
    {
        Psel = AD9833_PSEL1;
    }

    // 设置最终的波形输出，并解除复位状态
    AD9833_SetWave(WaveMode, Fsel, Psel);
}
