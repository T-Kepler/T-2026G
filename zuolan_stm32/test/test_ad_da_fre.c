/*
 * AD、DA、测频和FFT板级用法示例。
 * 这些函数需要在CubeMX初始化FMC、GPIO和USART后调用，不是PC端自动测试。
 */
#include "ad_measure.h"
#include "cmd_to_fun.h"
#include "da_output.h"
#include "freq_measure.h"
#include "key_app.h"
#include "my_fft.h"

void example_ad_capture_and_fft(void)
{
    float detected_frequency = 0.0f;
    float phase_difference = 0.0f;
    float fft_phase = 0.0f;
    float dominant_frequency = 0.0f;
    float dominant_magnitude = 0.0f;
    float sampling_frequency;

    /* 两路均按“已知信号频率”自动选择采样率并同步采集。 */
    vpp_adc_parallel(1000.0f, FREQ_MODE_SIGNAL,
                     1000.0f, FREQ_MODE_SIGNAL);

    (void)ad_detect_frequency(1, &detected_frequency);
    (void)ad_estimate_captured_frequency(1);
    sampling_frequency = get_adc_sampling_frequency(1);
    (void)get_adc_fft_sampling_frequency(1);
    (void)ad_measure_phase_difference(1000.0f, 0.0f, &phase_difference);

    fft_init();
    calculate_fft_spectrum(fifo_data1_f, FFT_LENGTH);
    (void)fft_get_phase(fifo_data1_f, sampling_frequency,
                        1000.0f, &fft_phase);
    (void)fft_find_dominant_frequency(fifo_data1_f, sampling_frequency,
                                      &dominant_frequency,
                                      &dominant_magnitude);
    output_fft_spectrum(sampling_frequency);
    (void)get_precise_peak_frequency(sampling_frequency);
    (void)calculate_thd(1000.0f, sampling_frequency);
    (void)calculate_thd_n(1000.0f, sampling_frequency);
    (void)calculate_sinad(1000.0f, sampling_frequency);

    /* 已有采样数据时，也可以按通道执行项目封装的一键分析。 */
    fft_analyze_ad_buffer(1);
}

void example_fpga_frequency_counter(void)
{
    fre_measure_ad1();
    fre_measure_ad2();

    /* 周期任务中调用，依次刷新freq1和freq2。 */
    freq_proc();
}

void example_da_output(void)
{
    DA_Init();
    DA_SetConfig(0, 1000.0f, 500, 0, WAVE_SINE);
    DA_SetConfig(1, 2000.0f, 500, 90, WAVE_SQUARE);

    DA_SetFREQ(0, 1500.0f);
    DA_SetAmp(0, 600);
    DA_SetPhase(0, 45);
    DA_SetWaveform(0, WAVE_TRIANGLE);
    (void)DA_GetFREQ(0);
    (void)DA_GetAmp(0);
    (void)DA_GetPhase(0);
    (void)DA_GetWaveform(0);
    DA_Apply_Settings();

    /* 上传一张STM32内置的128点波形表，并切换到自定义波形。 */
    DA_LoadPresetWaveform(DA_PRESET_SINE);
}

void example_fpga_control_register_diagnostic(void)
{
    /* 底层接口用于板卡联调；正常业务优先使用上面的高层接口。 */
    CTRL_INIT();
    DA_FPGA_START();
    DA_FPGA_STOP();

    AD_FREQ_CLR_ENABLE(1);
    AD_FREQ_CLR_DISABLE(1);
    AD_FREQ_START(1);
    AD_FREQ_STOP(1);
    AD_FREQ_SET(3);
    AD_FIFO_WRITE_ENABLE(3);
    AD_FIFO_WRITE_DISABLE(1);
    AD_FIFO_WRITE_DISABLE(2);
    AD_FIFO_READ_ENABLE(1);
    AD_FIFO_READ_DISABLE(1);
}
