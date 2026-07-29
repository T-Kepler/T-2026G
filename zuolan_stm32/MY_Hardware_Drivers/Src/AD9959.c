/***************************************************************************************************
 * @file    ad9959.c
 * @brief   AD9959 四通道DDS信号发生器 驱动实现文件
 * @author  左岚, 调库侠
 * @version V1.1
 * @date    2025-07-24
 *
 * @attention
 *
 * 本文件实现了ad9959.h中声明的函数。
 * 所有底层通信均通过GPIO模拟SPI完成。
 * 提供了丰富的API用于控制AD9959的各项功能。
 *
 ***************************************************************************************************/

#include "ad9959.h"
#include "bsp_system.h" // 包含 my_printf 等用户自定义函数

// --------------------------------- 内部辅助变量与函数 ---------------------------------

/**
 * @brief 寄存器长度数组
 * @note  存储每个寄存器地址对应的字节长度，用于SPI写入时确定循环次数。
 * 数组下标与寄存器地址一一对应。
 */
const u8 Reg_Len[25] = {
    1, 3, 2, 3, 4, 2, 3, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

#define AD9959_FR1_500MHZ       0xD00000UL
#define AD9959_CFR_SINGLE_TONE  0x000301UL

/**
 * @brief  通过GPIO模拟SPI发送一个字节
 * @param  byte 要发送的字节数据
 * @retval None
 * @note   MSB first (最高位在前)
 */
static void AD9959_Write_Byte_S(u8 byte)
{
    // 循环8次，发送一个字节的8位数据
    for (u8 i = 0; i < 8; i++)
    {
        // Match the known-good AD9959 bit-bang timing: data changes while SCLK is low.
        AD9959_SCK_L();

        // 从最高位开始判断
        if (byte & 0x80)
        {
            AD9959_SDIO0_H();
        }
        else
        {
            AD9959_SDIO0_L();
        }
        // 左移一位，准备发送下一位
        byte <<= 1;

        // 产生一个SCLK时钟脉冲
        AD9959_SCK_H();
    }
    // 将时钟线拉低，完成一个字节的传输
    AD9959_SCK_L();
}

/**
 * @brief  向AD9959指定寄存器写入数据
 * @param  reg_addr  目标寄存器的地址
 * @param  data      要写入的数据 (根据寄存器长度，最多32位)
 * @retval None
 * @note   这是一个内部核心函数，上层API都基于此函数实现。
 */
static void AD9959_Write_Reg(u8 reg_addr, u32 data)
{
    u8 nbyte = Reg_Len[reg_addr]; // 从数组中获取寄存器的字节长度

    AD9959_SCK_L();
    AD9959_CS_L(); // 拉低片选，开始通信

    // 步骤1: 发送8位的指令字节 (即寄存器地址)
    AD9959_Write_Byte_S(reg_addr);

    // 步骤2: 根据寄存器长度，发送N个字节的数据 (MSB first)
    for (int i = 0; i < nbyte; i++)
    {
        AD9959_Write_Byte_S((u8)(data >> (8 * (nbyte - 1 - i))));
    }

    AD9959_CS_H(); // 拉高片选，结束通信
}

// --------------------------------- 基础控制函数实现 ---------------------------------

/**
 * @brief  硬复位AD9959设备
 * @retval None
 * @note   RESET高电平有效，产生复位脉冲后拉低释放。
 */
void AD9959_Reset(void)
{
    AD9959_RST_L();
    HAL_Delay(1);
    AD9959_RST_H();
    HAL_Delay(1);
    AD9959_RST_L();
    HAL_Delay(1);
}

/**
 * @brief  触发I/O更新
 * @retval None
 * @note   在向寄存器写入数据后，必须调用此函数，数据才会从缓冲区加载到活动寄存器中生效。
 * 通过在IO_UPDATE引脚上产生一个上升沿来触发。
 */
void AD9959_IO_Update(void)
{
    AD9959_UP_L();
    HAL_Delay(1); // 延时确保电平稳定
    AD9959_UP_H();
    HAL_Delay(1);
    AD9959_UP_L();
}

/**
 * @brief  初始化AD9959设备
 * @retval None
 * @note   该函数完成AD9959上电后的基本配置。
 */
void AD9959_Init(void)
{
    // 初始化GPIO引脚状态
    AD9959_P0_L();
    AD9959_P1_L();
    AD9959_P2_L();
    AD9959_P3_L();
    AD9959_CS_H();
    AD9959_SCK_L();
    AD9959_UP_L();
    AD9959_PDC_L(); // 退出省电模式

    AD9959_Reset(); // 对设备进行硬复位

    // 配置CSR(0x00): 选择全部通道，后续通道寄存器初始化才能真正写入。
    AD9959_Write_Reg(CSR, ALL_CH_SELECT);

    // 配置FR1(0x01): 0xD00000 -> 1101 0000 0000 0000 0000 0000
    // [23]:VCO增益控制置1(高), [22:18]:PLL倍频设置为0b10100=20, 即25M*20=500M系统时钟
    // [16]:Charge pump control=01, [9:8]:Modulation level=00 (2-level)
    AD9959_Write_Reg(FR1, AD9959_FR1_500MHZ);

    // 配置FR2(0x02): 0x0000 -> 默认设置，如系统时钟使能等
    AD9959_Write_Reg(FR2, 0x00);

    // 配置CFR(0x03): 干净单音正弦，DAC满量程电流，关闭扫描/调制/自动清零相关位。
    AD9959_Write_Reg(CFR, AD9959_CFR_SINGLE_TONE);

    AD9959_IO_Update(); // 更新所有寄存器配置，使其生效
    AD9959_Select_Channel(CH0_SELECT);
}

/**
 * @brief  选择要操作的通道
 * @param  Channel 通道选择宏, 如 CH0_SELECT, CH1_SELECT, ALL_CH_SELECT
 * @retval None
 * @note   通过写入CSR寄存器来选择一个或多个通道。
 */
void AD9959_Select_Channel(u8 Channel)
{
    AD9959_Write_Reg(CSR, Channel);
}

// --------------------------------- 单点信号输出函数实现 ---------------------------------

/**
 * @brief  为当前选中通道设置输出频率
 * @param  Freq_Hz 目标频率 (单位: Hz)
 * @retval None
 */
void AD9959_Set_Freq(u32 Freq_Hz)
{
    // 根据公式 FTW = F_out * (2^32 / SYSCLK) 计算32位频率控制字
    u32 ftw = (u32)(Freq_Hz * ACC_FRE_FACTOR);
    AD9959_Write_Reg(CFTW0, ftw); // 写入通道频率调谐字寄存器0
}

/**
 * @brief  为当前选中通道设置输出相位
 * @param  Phase_Deg 目标相位 (单位: 度, 范围 0-360)
 * @retval None
 */
void AD9959_Set_Phase(float Phase_Deg)
{
    // 根据公式 POW = Phase_deg * (2^14 / 360) 计算14位相位偏移字
    // 寄存器是16位的，相位字占高14位
    u16 pow_val = (u16)(Phase_Deg * POW_REF);
    AD9959_Write_Reg(CPOW0, pow_val & 0x3FFF); // 写入通道相位偏移寄存器0 (屏蔽高2位)
}

/**
 * @brief  为当前选中通道设置输出幅度
 * @param  Amp 幅度值 (范围: 0-1023)
 * @retval None
 */
void AD9959_Set_Amp(u16 Amp)
{
    // 幅度寄存器ACR, [12]位是幅度倍增器使能位，[9:0]是10位幅度因子
    // 此处直接使能幅度倍增器
    if (Amp > 1023)
        Amp = 1023; // 限幅
    u32 acr_val = (1 << 12) | Amp;
    AD9959_Write_Reg(ACR, acr_val);
}

/**
 * @brief  单通道波形输出便捷函数
 * @param  Channel   通道宏 (CH0_SELECT ~ CH3_SELECT)
 * @param  Freq_Hz   输出频率 (Hz)
 * @param  Phase_Deg 输出相位 (度)
 * @param  Amp       输出幅度 (0-1023)
 * @retval None
 */
void AD9959_Single_Output(u8 Channel, u32 Freq_Hz, float Phase_Deg, u16 Amp)
{
    AD9959_Select_Channel(Channel); // 1. 选择通道
    AD9959_Set_Freq(Freq_Hz);       // 2. 设置频率
    AD9959_Set_Phase(Phase_Deg);    // 3. 设置相位
    AD9959_Set_Amp(Amp);            // 4. 设置幅度
}

// --------------------------------- 线性扫描函数实现 ---------------------------------

/**
 * @brief  配置CFR寄存器以使能线性扫描
 * @param  Channel   目标通道
 * @param  cfr_val   要写入CFR寄存器的值, 包含扫描模式等信息
 * @retval None
 * @note   这是一个辅助函数，用于统一设置线性扫描模式。
 */
void AD9959_Set_Linear_Sweep(u8 Channel, u32 cfr_val)
{
    AD9959_Select_Channel(Channel);
    // 使能线性扫描功能 (CFR[14]=1)
    // 清除调制相关的位 (CFR[23:22]), 并设置新的扫描模式
    u32 current_cfr = AD9959_CFR_SINGLE_TONE;
    cfr_val |= SWEEP_ENABLE;
    AD9959_Write_Reg(CFR, (current_cfr & 0x3FBDFF) | cfr_val);
}

/**
 * @brief  写线性扫描斜率寄存器 (LSRR)
 * @param  rise_srr  上升斜率更新时钟数 (1-255)
 * @param  fall_srr  下降斜率更新时钟数 (1-255)
 * @retval None
 * @note   斜率时间 = SRR * 4 / SYSCLK
 */
void AD9959_Write_LSRR(u8 rise_srr, u8 fall_srr)
{
    u32 lsrr_data = ((u32)fall_srr << 8) | rise_srr;
    AD9959_Write_Reg(LSRR, lsrr_data);
}

/**
 * @brief  写上升增量字 (RDW)
 * @param  rising_delta_word  上升增量数据
 * @retval None
 */
void AD9959_Write_RDW(u32 rising_delta_word)
{
    AD9959_Write_Reg(RDW, rising_delta_word);
}

/**
 * @brief  写下降增量字 (FDW)
 * @param  falling_delta_word 下降增量数据
 * @retval None
 */
void AD9959_Write_FDW(u32 falling_delta_word)
{
    AD9959_Write_Reg(FDW, falling_delta_word);
}

/**
 * @brief  配置线性扫频
 * @param  Channel     目标通道
 * @param  start_freq  起始频率 (Hz)
 * @param  end_freq    结束频率 (Hz)
 * @param  rise_srr    上升斜率时钟
 * @param  fall_srr    下降斜率时钟
 * @param  rise_step   上升步进频率 (Hz)
 * @param  fall_step   下降步进频率 (Hz)
 * @retval None
 */
void AD9959_Set_Freq_Sweep(u8 Channel, u32 start_freq, u32 end_freq, u8 rise_srr, u8 fall_srr, u32 rise_step, u32 fall_step)
{
    AD9959_Set_Linear_Sweep(Channel, 0); // CFR中设置扫频模式(0)
    AD9959_Write_LSRR(rise_srr, fall_srr);
    AD9959_Write_RDW((u32)(rise_step * ACC_FRE_FACTOR));
    AD9959_Write_FDW((u32)(fall_step * ACC_FRE_FACTOR));
    AD9959_Set_Freq(start_freq);                             // 起始频率写入CFTW0
    AD9959_Write_Reg(CW1, (u32)(end_freq * ACC_FRE_FACTOR)); // 结束频率写入CW1(Profile 0)
    AD9959_IO_Update();
}

/**
 * @brief  配置线性扫幅
 * @param  Channel     目标通道
 * @param  freq        载波频率 (Hz)
 * @param  start_amp   起始幅度 (0-1023)
 * @param  end_amp     结束幅度 (0-1023)
 * @param  rise_srr    上升斜率时钟
 * @param  fall_srr    下降斜率时钟
 * @param  rise_step   上升步进幅度
 * @param  fall_step   下降步进幅度
 * @retval None
 */
void AD9959_Set_Amp_Sweep(u8 Channel, u32 freq, u16 start_amp, u16 end_amp, u8 rise_srr, u8 fall_srr, u16 rise_step, u16 fall_step)
{
    AD9959_Set_Linear_Sweep(Channel, 1 << 22); // CFR中设置扫幅模式(1)
    AD9959_Set_Freq(freq);
    AD9959_Write_LSRR(rise_srr, fall_srr);
    AD9959_Write_RDW(rise_step); // 幅度增量字不需要因子转换，但需注意其在寄存器中的位置
    AD9959_Write_FDW(fall_step);
    AD9959_Set_Amp(start_amp);             // 起始幅度写入ACR
    AD9959_Write_Reg(CW1, (u32)(end_amp)); // 结束幅度写入CW1(Profile 0)
    AD9959_IO_Update();
}

/**
 * @brief  配置线性扫相
 * @param  Channel     目标通道
 * @param  freq        载波频率 (Hz)
 * @param  amp         载波幅度 (0-1023)
 * @param  start_phase 起始相位 (0-16383, 对应0-360度)
 * @param  end_phase   结束相位 (0-16383)
 * @param  rise_srr    上升斜率时钟
 * @param  fall_srr    下降斜率时钟
 * @param  rise_step   上升步进相位 (0-16383)
 * @param  fall_step   下降步进相位 (0-16383)
 * @retval None
 */
void AD9959_Set_Phase_Sweep(u8 Channel, u32 freq, u16 amp, u16 start_phase, u16 end_phase, u8 rise_srr, u8 fall_srr, u16 rise_step, u16 fall_step)
{
    AD9959_Set_Linear_Sweep(Channel, 2 << 22); // CFR中设置扫相模式(2)
    AD9959_Set_Freq(freq);
    AD9959_Set_Amp(amp);
    AD9959_Write_LSRR(rise_srr, fall_srr);
    AD9959_Write_RDW(rise_step);
    AD9959_Write_FDW(fall_step);
    AD9959_Write_Reg(CPOW0, start_phase & 0x3FFF);    // 起始相位写入CPOW0
    AD9959_Write_Reg(CW1, (u32)(end_phase & 0x3FFF)); // 结束相位写入CW1(Profile 0)
    AD9959_IO_Update();
}

// --------------------------------- 调制功能函数实现 ---------------------------------

/**
 * @brief  初始化调制功能
 * @param  Channel   目标通道
 * @param  mod_level 调制电平 (如 LEVEL_MOD_2, LEVEL_MOD_4 ...)
 * @param  mod_type  调制类型 (如 MOD_ASK, MOD_FSK, MOD_PSK)
 * @retval None
 */
void AD9959_Modulation_Init(u8 Channel, u8 mod_level, u8 mod_type)
{
    AD9959_Select_Channel(Channel);

    // 1. 在FR1中设置调制电平
    AD9959_Write_Reg(FR1, (0xD00000 | ((u32)mod_level << 8)));

    // 2. 在CFR中设置调制类型，并确保线性扫描被禁用
    u32 cfr_data = AD9959_CFR_SINGLE_TONE; // 基础配置保持单音正弦
    cfr_data &= ~(3 << 22);            // 清除旧的调制模式位
    cfr_data |= ((u32)mod_type << 16); // 设置新的调制模式
    cfr_data &= ~SWEEP_ENABLE;         // 确保线性扫描是禁用的
    AD9959_Write_Reg(CFR, cfr_data);

    AD9959_IO_Update();
}

/**
 * @brief  向Profile寄存器写入频率数据
 * @param  profile    Profile寄存器编号 (0-14)
 * @param  freq_hz    频率数据 (Hz)
 * @retval None
 */
void AD9959_Write_Profile_Freq(u8 profile, u32 freq_hz)
{
    if (profile > 14)
        return;
    u32 ftw = (u32)(freq_hz * ACC_FRE_FACTOR);
    AD9959_Write_Reg(PROFILE_ADDR_BASE + profile, ftw);
}

/**
 * @brief  向Profile寄存器写入幅度数据
 * @param  profile    Profile寄存器编号 (0-14)
 * @param  amp_data   幅度数据 (0-1023)
 * @retval None
 */
void AD9959_Write_Profile_Amp(u8 profile, u16 amp_data)
{
    if (profile > 14)
        return;
    if (amp_data > 1023)
        amp_data = 1023;
    // 在Profile寄存器中，幅度值位于[31:22]位，同时需要设置[12]位使能幅度控制
    u32 prof_amp_word = ((u32)amp_data << 22) | (1 << 12);
    AD9959_Write_Reg(PROFILE_ADDR_BASE + profile, prof_amp_word);
}

/**
 * @brief  向Profile寄存器写入相位数据
 * @param  profile      Profile寄存器编号 (0-14)
 * @param  phase_data   相位数据 (0-16383)
 * @retval None
 */
void AD9959_Write_Profile_Phase(u8 profile, u16 phase_data)
{
    if (profile > 14)
        return;
    // 在Profile寄存器中，相位值位于[31:18]位
    u32 prof_phase_word = (u32)(phase_data & 0x3FFF) << 18;
    AD9959_Write_Reg(PROFILE_ADDR_BASE + profile, prof_phase_word);
}

/**
 * @brief  配置FSK参数
 * @param  Channel      目标通道
 * @param  freq_array   频率数组指针 (包含基频和各Profile频率)
 * @retval None
 */
void AD9959_Set_FSK(u8 Channel, u32 *freq_array)
{
    AD9959_Select_Channel(Channel);
    AD9959_Set_Freq(freq_array[0]); // 设置基准频率 (对应Profile Pin全为低电平)
    // 循环写入15个Profile寄存器
    for (u8 i = 0; i < 15; i++)
    {
        AD9959_Write_Profile_Freq(i, freq_array[i + 1]);
    }
    AD9959_IO_Update();
}

/**
 * @brief  配置ASK参数
 * @param  Channel    目标通道
 * @param  freq       载波频率 (Hz)
 * @param  amp_array  幅度数组指针 (包含基幅和各Profile幅度)
 * @retval None
 */
void AD9959_Set_ASK(u8 Channel, u32 freq, u16 *amp_array)
{
    AD9959_Select_Channel(Channel);
    AD9959_Set_Freq(freq);        // 设置载波频率
    AD9959_Set_Amp(amp_array[0]); // 设置基准幅度
    for (u8 i = 0; i < 15; i++)
    {
        AD9959_Write_Profile_Amp(i, amp_array[i + 1]);
    }
    AD9959_IO_Update();
}

/**
 * @brief  配置PSK参数
 * @param  Channel      目标通道
 * @param  freq         载波频率 (Hz)
 * @param  phase_array  相位数组指针 (包含基相和各Profile相位)
 * @retval None
 */
void AD9959_Set_PSK(u8 Channel, u32 freq, u16 *phase_array)
{
    AD9959_Select_Channel(Channel);
    AD9959_Set_Freq(freq);                                // 设置载波频率
    AD9959_Set_Phase(phase_array[0] * 360.0f / 16384.0f); // 设置基准相位
    for (u8 i = 0; i < 15; i++)
    {
        AD9959_Write_Profile_Phase(i, phase_array[i + 1]);
    }
    AD9959_IO_Update();
}

// --------------------------------- 数字调制连续发送控制实现 ---------------------------------

// --- 全局调制状态变量 ---
static u8 ask_transmission_active = 0;
static u8 fsk_transmission_active = 0;
static u8 ask_initialized = 0;
static u8 fsk_initialized = 0;

// --- ASK调制状态机变量 ---
static u8 sending_ask = 0;                                     // 当前是否正在发送一个字节
static s8 ask_bit_index = 0;                                   // 当前发送的位索引 (从7到0)
static u32 ask_last_time = 0;                                  // 上次发送位的时间戳
static u32 ask_byte_interval_time = 0;                         // 字节发送间隔计时器
static u8 ask_data_index = 0;                                  // 当前在测试序列中的索引
static u8 ask_current_byte = 0;                                // 当前正在发送的字节
static u8 ask_in_byte_interval = 0;                            // 是否处于字节发送间隔
static const u8 ask_test_pattern[] = {0xB4, 0xCA, 0x55, 0xAA}; // ASK测试数据序列

// --- FSK调制状态机变量 ---
static u8 sending_fsk = 0;
static s8 fsk_bit_index = 0;
static u32 fsk_last_time = 0;
static u32 fsk_byte_interval_time = 0;
static u8 fsk_data_index = 0;
static u8 fsk_in_byte_interval = 0;
static const u8 fsk_test_pattern[] = {0xCA, 0x96, 0x33, 0xCC}; // FSK测试数据序列

/**
 * @brief 配置上电默认输出
 * @note  CH1/CH2/CH3共用P0输入的同一组2电平码元。
 */
void AD9959_Default_Output_Init(void)
{
    static u16 ask_amplitudes[16] = {0, 512};
    static u16 psk_phases[16] = {0, 8192};
    static u32 fsk_frequencies[16] = {1000000, 2000000};

    AD9959_Init();

    AD9959_Single_Output(CH0_SELECT, 1000, 0.0f, 500);
    AD9959_IO_Update();

    AD9959_Modulation_Init(CH1_SELECT, LEVEL_MOD_2, MOD_ASK);
    AD9959_Set_ASK(CH1_SELECT, 1000000, ask_amplitudes);

    AD9959_Modulation_Init(CH2_SELECT, LEVEL_MOD_2, MOD_PSK);
    AD9959_Set_PSK(CH2_SELECT, 1000000, psk_phases);
    AD9959_Select_Channel(CH2_SELECT);
    AD9959_Set_Amp(500);
    AD9959_IO_Update();

    AD9959_Modulation_Init(CH3_SELECT, LEVEL_MOD_2, MOD_FSK);
    AD9959_Set_FSK(CH3_SELECT, fsk_frequencies);
    AD9959_Select_Channel(CH3_SELECT);
    AD9959_Set_Amp(500);
    AD9959_IO_Update();

    AD9959_P0_L();
    ask_initialized = 1;
    ask_transmission_active = 1;
}

/**
 * @brief  启动ASK连续发送
 * @retval None
 */
void AD9959_Start_ASK_Transmission(void)
{
    // 定义2ASK调制的幅度数组 (基准+15个Profile)
    // 2电平调制由P0选择：P0=1代表'1'，P0=0代表'0'
    static u16 ask_amplitudes[16] = {
        0,                                                     // 基准幅度 (bit '0')
        512,                                                   // Profile 0 幅度 (bit '1')
        0, 512, 0, 512, 0, 512, 0, 512, 0, 512, 0, 512, 0, 512 // 其他Profile备用
    };

    if (!ask_initialized)
    {
        my_printf(&huart1, "Initializing 2-ASK on CH2...\r\n");
        // 初始化CH2的2-ASK调制模式，使用P0引脚控制
        AD9959_Modulation_Init(CH2_SELECT, LEVEL_MOD_2, MOD_ASK);
        HAL_Delay(10); // 等待配置稳定

        // 设置ASK参数: 通道2, 载波频率1MHz, 相位0
        AD9959_Set_ASK(CH2_SELECT, 1000000, ask_amplitudes);
        HAL_Delay(10);

        ask_initialized = 1;
    }

    AD9959_P0_L();
    ask_transmission_active = 1; // 激活发送任务
    my_printf(&huart1, "ASK continuous transmission started.\r\n");
}

/**
 * @brief  停止ASK发送
 * @retval None
 */
void AD9959_Stop_ASK_Transmission(void)
{
    ask_transmission_active = 0;
    sending_ask = 0;
    ask_in_byte_interval = 0;
    AD9959_P0_L(); // 确保profile引脚拉低，输出基准幅度(0)
    my_printf(&huart1, "ASK transmission stopped.\r\n");
}

/**
 * @brief  启动FSK连续发送
 * @retval None
 */
void AD9959_Start_FSK_Transmission(void)
{
    // 定义2FSK调制的频率数组 (基准+15个Profile)
    // Profile 0 (P3=1)代表 '1', 基准频率(P3=0)代表 '0'
    static u32 fsk_frequencies[16] = {
        800000,                                            // 基准频率 (bit '0')
        2000000,                                           // Profile 0 频率 (bit '1')
        800000, 2000000, 800000, 2000000, 800000, 2000000, // 其他备用
        800000, 2000000, 800000, 2000000, 800000, 2000000, 800000, 2000000};

    if (!fsk_initialized)
    {
        my_printf(&huart1, "Initializing 2-FSK on CH3...\r\n");
        // 初始化2-FSK调制模式，使用P3引脚控制
        AD9959_Modulation_Init(CH3_SELECT, LEVEL_MOD_2, MOD_FSK);
        HAL_Delay(10);

        // 设置FSK参数: 通道3
        AD9959_Set_FSK(CH3_SELECT, fsk_frequencies);
        HAL_Delay(10);

        fsk_initialized = 1;
    }

    fsk_transmission_active = 1; // 激活发送任务
    my_printf(&huart1, "FSK continuous transmission started.\r\n");
}

/**
 * @brief  停止FSK发送
 * @retval None
 */
void AD9959_Stop_FSK_Transmission(void)
{
    fsk_transmission_active = 0;
    sending_fsk = 0;
    fsk_in_byte_interval = 0;
    AD9959_P3_L(); // 确保profile引脚拉低，输出基准频率
    my_printf(&huart1, "FSK transmission stopped.\r\n");
}

/**
 * @brief  调制状态更新函数 (需要在主循环中周期性调用)
 * @retval None
 * @note   默认输出使用ASK状态机在P0产生共享码流，CH1/CH2/CH3同步调制。
 */
void AD9959_Modulation_State_Update(void)
{
    // --- CH1 ASK、CH2 PSK、CH3 FSK共享码流处理 ---
    if (ask_transmission_active && ask_initialized)
    {
        // 1. 检查是否处于字节发送间隔
        if (ask_in_byte_interval)
        {
            if (HAL_GetTick() - ask_byte_interval_time >= 100)
            {                             // 100ms字节间隔
                ask_in_byte_interval = 0; // 间隔结束，准备发送下一个字节
            }
        }
        // 2. 如果不处于间隔期，则处理数据发送
        else
        {
            // 2.1 如果当前没有在发送，则初始化一个新的字节发送任务
            if (!sending_ask)
            {
                sending_ask = 1;
                ask_bit_index = 7; // 从最高位开始
                ask_current_byte = ask_test_pattern[ask_data_index];
                ask_data_index = (ask_data_index + 1) % (sizeof(ask_test_pattern) / sizeof(ask_test_pattern[0]));
                ask_last_time = HAL_GetTick();
                my_printf(&huart1, "ASK sending: 0x%02X -> ", ask_current_byte);
            }

            // 2.2 检查是否到了发送下一位的时间 (码元速率控制)
            if (sending_ask && (HAL_GetTick() - ask_last_time >= 300))
            {                                                           // 300ms一个码元
                if (ask_current_byte & (1 << ask_bit_index))
                {
                    AD9959_P0_H(); // 发送'1'
                    my_printf(&huart1, "1");
                }
                else
                {
                    AD9959_P0_L(); // 发送'0'
                    my_printf(&huart1, "0");
                }

                ask_bit_index--;
                ask_last_time = HAL_GetTick();

                // 2.3 检查一个字节是否发送完毕
                if (ask_bit_index < 0)
                {
                    sending_ask = 0; // 当前字节发送完成
                    AD9959_P0_L();   // 结束后将电平拉低
                    my_printf(&huart1, " ...completed\r\n");

                    // 开始字节间隔计时
                    ask_in_byte_interval = 1;
                    ask_byte_interval_time = HAL_GetTick();
                }
            }
        }
    }

    // --- FSK调制状态机处理 (逻辑同ASK) ---
    if (fsk_transmission_active && fsk_initialized)
    {
        if (fsk_in_byte_interval)
        {
            if (HAL_GetTick() - fsk_byte_interval_time >= 100)
            { // 100ms字节间隔
                fsk_in_byte_interval = 0;
            }
        }
        else
        {
            if (!sending_fsk)
            {
                sending_fsk = 1;
                fsk_bit_index = 7;
                u8 current_byte = fsk_test_pattern[fsk_data_index];
                fsk_data_index = (fsk_data_index + 1) % (sizeof(fsk_test_pattern) / sizeof(fsk_test_pattern[0]));
                fsk_last_time = HAL_GetTick();
                my_printf(&huart1, "FSK sending: 0x%02X -> ", current_byte);
            }

            if (sending_fsk && (HAL_GetTick() - fsk_last_time >= 400))
            { // 400ms一个码元
                u8 current_byte = fsk_test_pattern[fsk_data_index - 1];
                if (current_byte & (1 << fsk_bit_index))
                {
                    AD9959_P3_H(); // 发送'1'
                    my_printf(&huart1, "1");
                }
                else
                {
                    AD9959_P3_L(); // 发送'0'
                    my_printf(&huart1, "0");
                }

                fsk_bit_index--;
                fsk_last_time = HAL_GetTick();

                if (fsk_bit_index < 0)
                {
                    sending_fsk = 0;
                    AD9959_P3_L();
                    my_printf(&huart1, " ...completed\r\n");
                    fsk_in_byte_interval = 1;
                    fsk_byte_interval_time = HAL_GetTick();
                }
            }
        }
    }
}

/**
 * @brief  查询ASK发送任务是否激活
 * @return u8 1:激活, 0:未激活
 */
u8 AD9959_Is_ASK_Active(void)
{
    return ask_transmission_active;
}

/**
 * @brief  查询FSK发送任务是否激活
 * @return u8 1:激活, 0:未激活
 */
u8 AD9959_Is_FSK_Active(void)
{
    return fsk_transmission_active;
}

// --------------------------------- 用户测试函数 ---------------------------------

// 定义一些测试用的全局变量
u32 pid_vin = 38;
u16 Phase = 0;
u8 mdoe_flag = 0; // 0: CW模式, 1: AM模式 (示例)

/**
 * @brief  用户自定义测试逻辑函数
 * @retval None
 * @note   此函数供用户进行快速的功能验证。
 */
void AD9959_proc(void)
{
    // 示例:
    // 通道0: 输出一个固定信号，幅度由pid_vin控制，相位可变
    AD9959_Single_Output(CH0_SELECT, 2000000, (float)Phase, (u16)pid_vin);

    // 通道1: 根据mdoe_flag切换输出
    if (mdoe_flag == 0)
    {
        // CW模式，关闭通道1输出 (幅度为0)
        AD9959_Single_Output(CH1_SELECT, 2000, 0, 0);
    }
    else
    {
        // AM模式(示例)，输出一个固定信号
        AD9959_Single_Output(CH1_SELECT, 2000, 0, 200);
    }

    // 通道2 & 3: 输出固定的高幅信号
    AD9959_Single_Output(CH2_SELECT, 2000000, 0, 1023);
    AD9959_Single_Output(CH3_SELECT, 100000, 0, 1023);

    // !!! 关键步骤: 每次修改参数后，必须调用IO_Update使设置生效
    AD9959_IO_Update();

    // 翻转一个IO口，用于在示波器上观察proc函数的执行频率
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_2);
}

// --------------------------------- 底层GPIO控制函数实现 ---------------------------------
// 说明: 以下函数为对STM32 HAL库GPIO操作的简单封装，使其函数名更具可读性。
//       具体的引脚定义在 main.h 或相关配置文件中。

void AD9959_P0_H(void) { HAL_GPIO_WritePin(AD9959_P0_GPIO_Port, AD9959_P0_Pin, GPIO_PIN_SET); }
void AD9959_P0_L(void) { HAL_GPIO_WritePin(AD9959_P0_GPIO_Port, AD9959_P0_Pin, GPIO_PIN_RESET); }
void AD9959_P1_H(void) { HAL_GPIO_WritePin(AD9959_P1_GPIO_Port, AD9959_P1_Pin, GPIO_PIN_SET); }
void AD9959_P1_L(void) { HAL_GPIO_WritePin(AD9959_P1_GPIO_Port, AD9959_P1_Pin, GPIO_PIN_RESET); }
void AD9959_P2_H(void) { HAL_GPIO_WritePin(AD9959_P2_GPIO_Port, AD9959_P2_Pin, GPIO_PIN_SET); }
void AD9959_P2_L(void) { HAL_GPIO_WritePin(AD9959_P2_GPIO_Port, AD9959_P2_Pin, GPIO_PIN_RESET); }
void AD9959_P3_H(void) { HAL_GPIO_WritePin(AD9959_P3_GPIO_Port, AD9959_P3_Pin, GPIO_PIN_SET); }
void AD9959_P3_L(void) { HAL_GPIO_WritePin(AD9959_P3_GPIO_Port, AD9959_P3_Pin, GPIO_PIN_RESET); }

void AD9959_UP_H(void) { HAL_GPIO_WritePin(AD9959_UP_GPIO_Port, AD9959_UP_Pin, GPIO_PIN_SET); }
void AD9959_UP_L(void) { HAL_GPIO_WritePin(AD9959_UP_GPIO_Port, AD9959_UP_Pin, GPIO_PIN_RESET); }
void AD9959_PDC_H(void) { HAL_GPIO_WritePin(AD9959_PDC_GPIO_Port, AD9959_PDC_Pin, GPIO_PIN_SET); }
void AD9959_PDC_L(void) { HAL_GPIO_WritePin(AD9959_PDC_GPIO_Port, AD9959_PDC_Pin, GPIO_PIN_RESET); }
void AD9959_RST_H(void) { HAL_GPIO_WritePin(AD9959_RST_GPIO_Port, AD9959_RST_Pin, GPIO_PIN_SET); }
void AD9959_RST_L(void) { HAL_GPIO_WritePin(AD9959_RST_GPIO_Port, AD9959_RST_Pin, GPIO_PIN_RESET); }

void AD9959_CS_H(void) { HAL_GPIO_WritePin(AD9959_CS_GPIO_Port, AD9959_CS_Pin, GPIO_PIN_SET); }
void AD9959_CS_L(void) { HAL_GPIO_WritePin(AD9959_CS_GPIO_Port, AD9959_CS_Pin, GPIO_PIN_RESET); }
void AD9959_SCK_H(void) { HAL_GPIO_WritePin(AD9959_SCK_GPIO_Port, AD9959_SCK_Pin, GPIO_PIN_SET); }
void AD9959_SCK_L(void) { HAL_GPIO_WritePin(AD9959_SCK_GPIO_Port, AD9959_SCK_Pin, GPIO_PIN_RESET); }

void AD9959_SDIO0_H(void) { HAL_GPIO_WritePin(AD9959_SDIO0_GPIO_Port, AD9959_SDIO0_Pin, GPIO_PIN_SET); }
void AD9959_SDIO0_L(void) { HAL_GPIO_WritePin(AD9959_SDIO0_GPIO_Port, AD9959_SDIO0_Pin, GPIO_PIN_RESET); }
void AD9959_SDIO1_H(void) { HAL_GPIO_WritePin(AD9959_SDIO1_GPIO_Port, AD9959_SDIO1_Pin, GPIO_PIN_SET); }
void AD9959_SDIO1_L(void) { HAL_GPIO_WritePin(AD9959_SDIO1_GPIO_Port, AD9959_SDIO1_Pin, GPIO_PIN_RESET); }
void AD9959_SDIO2_H(void) { HAL_GPIO_WritePin(AD9959_SDIO2_GPIO_Port, AD9959_SDIO2_Pin, GPIO_PIN_SET); }
void AD9959_SDIO2_L(void) { HAL_GPIO_WritePin(AD9959_SDIO2_GPIO_Port, AD9959_SDIO2_Pin, GPIO_PIN_RESET); }
void AD9959_SDIO3_H(void) { HAL_GPIO_WritePin(AD9959_SDIO3_GPIO_Port, AD9959_SDIO3_Pin, GPIO_PIN_SET); }
void AD9959_SDIO3_L(void) { HAL_GPIO_WritePin(AD9959_SDIO3_GPIO_Port, AD9959_SDIO3_Pin, GPIO_PIN_RESET); }
