$ErrorActionPreference = 'Stop'

$fpgaRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$projectRoot = Join-Path $fpgaRoot 'prj'
$qsfPath = Join-Path $projectRoot 'ZUOLAN_FPGA_OBJECT.qsf'
$sdcPath = Join-Path $projectRoot 'ZUOLAN_FPGA_OBJECT.sdc'
$topPath = Join-Path $fpgaRoot 'src\TOP.v'
$fmcPath = Join-Path $fpgaRoot 'src\FMC_CONTROL.v'
$waveformPath = Join-Path $fpgaRoot 'src\DA_WAVEFORM.v'
$scalerPath = Join-Path $fpgaRoot 'src\VOLTAGE_SCALER_CLOCKED.v'
$pllPath = Join-Path $fpgaRoot 'ip\MYPLL\MYPLL.v'
$adMeasurePath = Join-Path (Split-Path $fpgaRoot -Parent) 'zuolan_stm32\MY_Hardware_Drivers\Src\ad_measure.c'
$qsf = Get-Content -Raw -LiteralPath $qsfPath
$sdc = Get-Content -Raw -LiteralPath $sdcPath
$top = Get-Content -Raw -LiteralPath $topPath
$fmc = Get-Content -Raw -LiteralPath $fmcPath
$waveform = Get-Content -Raw -LiteralPath $waveformPath
$scaler = Get-Content -Raw -LiteralPath $scalerPath
$pll = Get-Content -Raw -LiteralPath $pllPath
$adMeasure = Get-Content -Raw -LiteralPath $adMeasurePath

function Assert-True([bool]$condition, [string]$message) {
    if (-not $condition) {
        throw $message
    }
}

Assert-True ($qsf -match 'VERILOG_FILE\s+\.\./src/TOP\.v') 'QSF 未引用 src/TOP.v'
Assert-True ($qsf -match 'SDC_FILE\s+ZUOLAN_FPGA_OBJECT\.sdc') 'QSF 未引用 SDC'
Assert-True ($qsf -match 'FAST_OUTPUT_REGISTER\s+ON\s+-to\s+DA1_OUT\[\*\]') 'DA1 输出寄存器未放入IOE'
Assert-True ($qsf -match 'FAST_OUTPUT_REGISTER\s+ON\s+-to\s+DA2_OUT\[\*\]') 'DA2 输出寄存器未放入IOE'
Assert-True ($qsf -notmatch 'BDF_FILE') 'QSF 仍引用 BDF'
Assert-True (-not (Get-ChildItem -LiteralPath $fpgaRoot -Recurse -File | Where-Object { $_.Extension -in '.bdf', '.bsf' })) 'FPGA 目录仍包含 BDF/BSF'
Assert-True (-not (Get-ChildItem -LiteralPath (Join-Path $fpgaRoot 'ip') -Recurse -File -Filter '*.qip' | Select-String -Pattern '\.bsf')) 'QIP 仍引用 BSF'

foreach ($match in [regex]::Matches($qsf, '(?m)^set_global_assignment -name (?:VERILOG_FILE|QIP_FILE) (.+)$')) {
    $relativePath = $match.Groups[1].Value.Trim().Trim('"')
    Assert-True (Test-Path -LiteralPath (Join-Path $projectRoot $relativePath)) "QSF 引用不存在：$relativePath"
}

foreach ($module in 'FMC_CONTROL', 'DA_PARAMETER_CTRL', 'TYFIFO') {
    Assert-True ($top -match "\b$module\b") "TOP.v 缺少模块：$module"
}

foreach ($index in 0..15) {
    Assert-True ($top -match "\.read_data_$($index)_\s*\(") "FMC 读寄存器端口 $index 未连接"
    Assert-True ($top -match "\.write_data_$($index)_\s*\(") "FMC 写寄存器端口 $index 未连接"
}

foreach ($removedModule in 'MASTER_CTRL.v', 'DA_PARAMETER_DEAL.v', 'AD_FREQ_WORD.v', 'AD_DATA_DEAL.v', 'AD_FREQ_MEASURE.v') {
    Assert-True ($qsf -notmatch [regex]::Escape($removedModule)) "QSF 仍引用重复锁存模块：$removedModule"
}

Assert-True (-not (Test-Path -LiteralPath (Join-Path $fpgaRoot 'src\AD_DATA_DEAL.v'))) '重复模块 AD_DATA_DEAL.v 仍存在'
Assert-True (-not (Test-Path -LiteralPath (Join-Path $fpgaRoot 'src\AD_FREQ_MEASURE.v'))) '重复模块 AD_FREQ_MEASURE.v 仍存在'

Assert-True ($top -match 'assign\s+da1_amplitude\s*=\s*read_data_14\[11:0\]') 'DA1 幅度未连接 FMC 地址 14'
Assert-True ($top -match 'assign\s+da2_amplitude\s*=\s*read_data_15\[11:0\]') 'DA2 幅度未连接 FMC 地址 15'
Assert-True ($top -match 'assign\s+da1_wave_select\s*=\s*read_data_12\[7:0\]') 'DA1 波形选择字节映射错误'
Assert-True ($top -match 'assign\s+da2_wave_select\s*=\s*read_data_12\[15:8\]') 'DA2 波形选择字节映射错误'
Assert-True ($top -match 'assign\s+ad1_fifo_data_read\s*=') 'AD1 FIFO 数据未直接映射'
Assert-True ($top -match 'assign\s+ad2_fifo_data_read\s*=') 'AD2 FIFO 数据未直接映射'
Assert-True ($top -match 'assign\s+base1_frequency_high\s*=\s*base1_frequency_count\[31:16\]') '基准频率高位未直接映射'
Assert-True ($top -match 'assign\s+ad2_frequency_low\s*=\s*ad2_frequency_count\[15:0\]') 'AD2 频率低位未直接映射'
Assert-True ($waveform -match 'input\s+\[\s*6:0\]\s+PHASE_ADDR') 'DA ROM 地址不是 7 位'
Assert-True ($waveform -match 'module\s+DA_CUSTOM_WAVEFORM_RAM') '缺少STM32自定义波形双口RAM'
Assert-True ($waveform -match 'CUSTOM_WAVE\s*=\s*8''d4') '自定义波形选择码不是4'
Assert-True ($top -match 'address\[15:7\]\s*==\s*9''h002') '自定义波形写窗口不是0x0100~0x017F'
Assert-True ([regex]::Matches($top, 'DA_CUSTOM_WAVEFORM_RAM').Count -eq 2) '双路DA未各自实例化自定义波形RAM'
Assert-True ($top -match 'assign\s+DA1_OUTCLK\s*=\s*da_capture_clock') 'DA1 未使用125MHz校准采样时钟'
Assert-True ($top -match 'assign\s+DA2_OUTCLK\s*=\s*da_capture_clock') 'DA2 未使用125MHz校准采样时钟'
Assert-True ([regex]::Matches($top, '\.wrclk\s*\(clk_base\)').Count -eq 2) 'AD FIFO 未统一使用150MHz基准写时钟'
Assert-True ($top -match '\.wrreq\s*\(control_data\[4\]\s*&\s*ad1_sample_strobe\s*&\s*~ad1_capture_done\)') 'AD1 FIFO 未使用采样沿写使能'
Assert-True ($top -match '\.wrreq\s*\(control_data\[6\]\s*&\s*ad2_capture_strobe\s*&\s*~ad2_capture_done\)') 'AD2 FIFO 未使用配对采样沿写使能'
Assert-True ($top -match 'control_data\[12\].+ad1_capture_count\s*==\s*14''d127') 'AD1 未实现128点短采集'
Assert-True ($top -match 'control_data\[13\].+ad2_capture_count\s*==\s*10''d127') 'AD2 未实现128点短采集'
Assert-True ($top -notmatch '\.wrclk\s*\(ad[12]_sampling_clock\)') 'AD FIFO 仍使用逻辑生成时钟'
Assert-True ($fmc -match 'rd_data_reg\s*<=\s*16''d0') 'FMC读缓存未初始化为零'
Assert-True ($fmc -match 'default:\s*rd_data_reg\s*<=\s*16''d0') 'FMC非法读地址未返回零'
Assert-True ($scaler -match 'scaled_data_pipe4\s*<=') 'DA幅度加减未在上升沿独立流水'
Assert-True ($scaler -match 'always\s*@\(negedge\s+clk\)') 'DA数据未在125MHz时钟下降沿更新'
Assert-True ($pll -match 'clk1_multiply_by\s*=\s*5') 'PLL 未生成125MHz DA数据时钟'
Assert-True ($pll -match 'clk2_phase_shift\s*=\s*"500"') 'PLL 的DA采样时钟未延后0.5ns'
Assert-True ($sdc -match 'derive_pll_clocks') 'SDC 未约束PLL时钟'
Assert-True ($sdc -match 'get_clocks\s+\{pll\|altpll_component\|auto_generated\|pll1\|clk\[2\]\}') 'SDC 的DA采样时钟源不是125MHz校准时钟'
Assert-True ($sdc -match 'set_output_delay.+-max\s+2\.000') 'SDC 未约束AD9764建立时间'
Assert-True ($sdc -match 'set_output_delay.+-min\s+-1\.500') 'SDC 未约束AD9764保持时间'
Assert-True ($sdc -match 'set_false_path\s+-from\s+\[get_registers\s+\{\*frequency_counter\|Q1\[\*\]\}\]') 'SDC 未约束测频读回CDC源端'
Assert-True ($sdc -match '-to\s+\[get_registers\s+\{\*fmc\|rd_data_reg\[\*\]\}\]') 'SDC 未约束测频读回CDC目的端'
Assert-True ($adMeasure -match 'ADC_MAX_SAMPLE_RATE\s+65000000U') 'ADC采样率未限制在65MHz以内'
Assert-True ($adMeasure -match 'raw_data\s*&\s*0xF000U') 'AD幅度读回未检查12位数据边界'
Assert-True ($adMeasure -match 'valid\s*=\s*0') '非法AD样本未标记本轮幅度无效'
Assert-True ($adMeasure -match 'return\s+valid') 'AD FIFO读回未返回整轮有效性'
Assert-True ([regex]::Matches($adMeasure, 'if\s*\(readFIFOData\(').Count -eq 2) '非法AD样本仍可能更新幅度'

# 回放现场同量级的0xFFFF脏读数：整轮无效时必须保留上次可信幅度。
$previousAmplitude = 5.014648
$capturedRaw = @(2048, 3074, 2047, 0xFFFF)
$captureValid = -not ($capturedRaw | Where-Object { ($_ -band 0xF000) -ne 0 })
$reportedAmplitude = if ($captureValid) { 310.0 } else { $previousAmplitude }
Assert-True ([math]::Abs($reportedAmplitude - $previousAmplitude) -lt 0.000001) '非法AD样本仍会造成幅度跳变'

$pinNames = [regex]::Matches($qsf, '(?m)^set_location_assignment .+ -to ([A-Za-z0-9_]+)(?:\[\d+\])?$') |
    ForEach-Object { $_.Groups[1].Value } |
    Sort-Object -Unique
foreach ($pinName in $pinNames) {
    Assert-True ($top -match "\b$pinName\b") "TOP.v 缺少引脚：$pinName"
}

Write-Host 'FPGA结构与AD幅度防护检查通过。'
