/* 单AD9833板级示例。 */
#include "AD9833.h"

void example_ad9833_output(void)
{
    AD9833_Init();
    AD9833_SetFrequency(AD9833_REG_FREQ0, 1000.0);
    AD9833_SetPhase(AD9833_REG_PHASE0, 0U);
    AD9833_SetWave(AD9833_OUT_SINUS, AD9833_FSEL0, AD9833_PSEL0);
    AD9833_Setup(AD9833_REG_FREQ0, 1000.0,
                 AD9833_REG_PHASE0, 0U, AD9833_OUT_SINUS);
}

void example_ad9833_register_diagnostic(void)
{
    /* 原始寄存器写入只用于驱动联调，写完后恢复正常输出配置。 */
    AD9833_WriteData(AD9833_REG_CMD | AD9833_B28 | AD9833_RESET);
    AD9833_Setup(AD9833_REG_FREQ0, 1000.0,
                 AD9833_REG_PHASE0, 0U, AD9833_OUT_SINUS);
}
