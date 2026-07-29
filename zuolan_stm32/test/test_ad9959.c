/* AD9959板级示例：各函数按用途独立调用，不要在一次上电流程中全部执行。 */
#include "AD9959.h"

void example_ad9959_single_output(void)
{
    AD9959_Init();
    AD9959_Reset();
    AD9959_Select_Channel(CH0_SELECT);
    AD9959_Set_Freq(1000U);
    AD9959_Set_Phase(0.0f);
    AD9959_Set_Amp(500U);
    AD9959_IO_Update();

    /* 便捷接口会一次写入指定通道的频率、相位和幅度。 */
    AD9959_Single_Output(CH1_SELECT, 2000U, 90.0f, 500U);
    AD9959_IO_Update();
}

void example_ad9959_sweep(void)
{
    /* 底层扫描寄存器接口，适合观察单步配置。 */
    AD9959_Set_Linear_Sweep(CH0_SELECT, 0U);
    AD9959_Write_LSRR(1U, 1U);
    AD9959_Write_RDW(100U);
    AD9959_Write_FDW(100U);

    AD9959_Set_Freq_Sweep(CH0_SELECT, 1000U, 10000U,
                          1U, 1U, 100U, 100U);
    AD9959_Set_Amp_Sweep(CH1_SELECT, 1000U, 100U, 800U,
                         1U, 1U, 10U, 10U);
    AD9959_Set_Phase_Sweep(CH2_SELECT, 1000U, 500U, 0U, 8192U,
                           1U, 1U, 64U, 64U);
}

void example_ad9959_modulation(void)
{
    static u32 fsk_frequency[16] = {1000000U, 2000000U};
    static u16 ask_amplitude[16] = {0U, 512U};
    static u16 psk_phase[16] = {0U, 8192U};

    AD9959_Modulation_Init(CH1_SELECT, LEVEL_MOD_2, MOD_ASK);
    AD9959_Write_Profile_Freq(0U, 2000000U);
    AD9959_Write_Profile_Amp(0U, 512U);
    AD9959_Write_Profile_Phase(0U, 8192U);
    AD9959_Set_FSK(CH3_SELECT, fsk_frequency);
    AD9959_Set_ASK(CH1_SELECT, 1000000U, ask_amplitude);
    AD9959_Set_PSK(CH2_SELECT, 1000000U, psk_phase);

    AD9959_Start_ASK_Transmission();
    (void)AD9959_Is_ASK_Active();
    AD9959_Modulation_State_Update();
    AD9959_Stop_ASK_Transmission();

    AD9959_Start_FSK_Transmission();
    (void)AD9959_Is_FSK_Active();
    AD9959_Modulation_State_Update();
    AD9959_Stop_FSK_Transmission();
}

void example_ad9959_project_default(void)
{
    AD9959_Default_Output_Init();
    AD9959_proc();
}

void example_ad9959_gpio_diagnostic(void)
{
    /* 仅用于示波器检查引脚；调用会短暂改变DDS状态。 */
    AD9959_CS_H();
    AD9959_CS_L();
    AD9959_CS_H();
    AD9959_SCK_L();
    AD9959_SCK_H();
    AD9959_SDIO0_L();
    AD9959_SDIO0_H();
    AD9959_SDIO1_L();
    AD9959_SDIO1_H();
    AD9959_SDIO2_L();
    AD9959_SDIO2_H();
    AD9959_SDIO3_L();
    AD9959_SDIO3_H();
    AD9959_P0_L();
    AD9959_P0_H();
    AD9959_P1_L();
    AD9959_P1_H();
    AD9959_P2_L();
    AD9959_P2_H();
    AD9959_P3_L();
    AD9959_P3_H();
    AD9959_UP_L();
    AD9959_UP_H();
    AD9959_PDC_H();
    AD9959_PDC_L();
    AD9959_RST_H();
    AD9959_RST_L();
}
