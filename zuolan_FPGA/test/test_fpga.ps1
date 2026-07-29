$ErrorActionPreference = 'Stop'

$fpgaRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$srcRoot = Join-Path $fpgaRoot 'src'
$qsfPath = Join-Path $fpgaRoot 'prj\ZUOLAN_FPGA_OBJECT.qsf'
$topPath = Join-Path $srcRoot 'TOP.v'

function Assert-True([bool]$condition, [string]$message) {
    if (-not $condition) {
        throw $message
    }
}

function Assert-Matches([string]$source, [string[]]$patterns, [string]$moduleName) {
    foreach ($pattern in $patterns) {
        Assert-True ($source -match $pattern) "$moduleName 缺少预期实现：$pattern"
    }
}

# 先复用同目录的引脚、时序、跨层寄存器和关键连线检查。
& (Join-Path $PSScriptRoot 'test_pure_verilog.ps1')

$qsf = Get-Content -Raw -LiteralPath $qsfPath
$top = Get-Content -Raw -LiteralPath $topPath
$sources = @{}
$discoveredModules = @{}

foreach ($sourceFile in Get-ChildItem -LiteralPath $srcRoot -File -Filter '*.v') {
    $source = Get-Content -Raw -LiteralPath $sourceFile.FullName
    $sources[$sourceFile.Name] = $source

    $modules = [regex]::Matches($source, '(?m)^\s*module\s+([A-Za-z_][A-Za-z0-9_]*)\b')
    $endmoduleCount = [regex]::Matches($source, '(?m)^\s*endmodule\b').Count
    Assert-True ($modules.Count -gt 0) "$($sourceFile.Name) 未声明模块"
    Assert-True ($modules.Count -eq $endmoduleCount) "$($sourceFile.Name) 的 module/endmodule 数量不一致"
    Assert-True ($qsf -match [regex]::Escape("../src/$($sourceFile.Name)")) "QSF 未引用 $($sourceFile.Name)"

    foreach ($module in $modules) {
        $moduleName = $module.Groups[1].Value
        Assert-True (-not $discoveredModules.ContainsKey($moduleName)) "模块重复声明：$moduleName"
        $discoveredModules[$moduleName] = $sourceFile.Name
        if ($moduleName -ne 'TOP') {
            Assert-True ($top -match "\b$([regex]::Escape($moduleName))\b") "TOP.v 未使用模块：$moduleName"
        }
    }
}

# 每个自研模块至少保留一组功能结构检查；新增模块时必须同步补到这里。
$moduleChecks = [ordered]@{
    'TOP' = @(
        'module\s+TOP\b',
        'assign\s+DA1_OUTCLK\s*=\s*da_capture_clock',
        'assign\s+DA2_OUTCLK\s*=\s*da_capture_clock',
        'reg\s+\[11:0\]\s+ad1_input_pipe1',
        'reg\s+\[11:0\]\s+ad1_input_pipe2',
        'reg\s+\[11:0\]\s+ad2_input_pipe1',
        'reg\s+\[11:0\]\s+ad2_input_pipe2',
        'ad1_input_pipe1\s*<=\s*AD1_INPUT',
        'ad1_input_pipe2\s*<=\s*ad1_input_pipe1',
        'ad2_input_pipe1\s*<=\s*AD2_INPUT',
        'ad2_input_pipe2\s*<=\s*ad2_input_pipe1',
        '\.data\s*\(ad1_input_pipe2\)',
        '\.data\s*\(ad2_input_pipe2\)'
    )
    'test' = @(
        'assign\s+new_data\s*=\s*old_data'
    )
    'FMC_CONTROL' = @(
        'assign\s+fmc_wr_en\s*=',
        'assign\s+fmc_rd_en\s*=',
        '16''h000F:\s*read_data_15__reg\s*<=\s*fpga_db',
        'default:\s*rd_data_reg\s*<=\s*16''d0'
    )
    'DA_PARAMETER_CTRL' = @(
        'accumulator_a\s*<=\s*accumulator_a\s*\+\s*frequency_word_a',
        'accumulator_b\s*<=\s*accumulator_b\s*\+\s*frequency_word_b',
        'assign\s+COUT_A_FINAL\s*=\s*accumulator_a\[31\s*-:\s*PHASE_WIDTH\]\s*\+\s*phase_offset_a'
    )
    'DA_WAVEFORM' = @(
        'CUSTOM_WAVE:\s*selected_wave_data\s*=\s*CUSTOM_DATA',
        'default:\s*selected_wave_data\s*=\s*sin_data',
        'wave_data_pipe2\s*<=\s*wave_data_pipe1'
    )
    'DA_CUSTOM_WAVEFORM_RAM' = @(
        'reg\s+\[13:0\]\s+memory\s*\[0:127\]',
        'if\s*\(WRITE_EN\)\s*memory\[WRITE_ADDR\]\s*<=\s*WRITE_DATA',
        'READ_DATA\s*<=\s*memory\[READ_ADDR\]'
    )
    'VOLTAGE_SCALER_CLOCKED' = @(
        'voltage_mv\s*>\s*MAX_VOLTAGE_MV',
        'always\s*@\(posedge\s+clk\)',
        'always\s*@\(negedge\s+clk\)',
        'enable_negedge\s*<=\s*enable',
        'if\s*\(!enable_negedge\)\s*scaled_data\s*<=\s*HALF_ROM_MAX'
    )
    'FREQ_DEV' = @(
        'if\s*\(PHASE_LOAD\)',
        'ACC\s*<=\s*ACC\s*\+\s*FREQ_WORD',
        'FREQ_WORD\s*<=\s*\{FREQH_W,\s*FREQL_W\}'
    )
    'CNT32' = @(
        'if\s*\(!CLR\)',
        'else\s+if\s*\(CLKEN\)',
        'if\s*\(!CLRBASE\)',
        'else\s+if\s*\(CLKBASEEN\)'
    )
    'G_AD_FIFO' = @(
        'lpm_numwords\s*=\s*16384',
        'lpm_width\s*=\s*12',
        'lpm_widthu\s*=\s*14'
    )
}

foreach ($moduleName in $discoveredModules.Keys) {
    Assert-True ($moduleChecks.Contains($moduleName)) "模块 $moduleName 缺少 test 中的功能检查"
}
foreach ($moduleName in $moduleChecks.Keys) {
    Assert-True ($discoveredModules.ContainsKey($moduleName)) "测试仍引用已删除模块：$moduleName"
    $sourceFileName = $discoveredModules[$moduleName]
    Assert-Matches $sources[$sourceFileName] $moduleChecks[$moduleName] $moduleName
}

# 生成 IP 不作为自研模块重复测试，只校验本项目依赖的配置和波形数据可用。
$generatedIpChecks = [ordered]@{
    'MYPLL' = @('clk0_multiply_by\s*=\s*3', 'clk1_multiply_by\s*=\s*5', 'clk2_phase_shift\s*=\s*"500"')
    'TYFIFO' = @('lpm_numwords\s*=\s*1024', 'lpm_width\s*=\s*12', 'lpm_showahead\s*=\s*"OFF"')
}
foreach ($ipName in $generatedIpChecks.Keys) {
    $ipPath = Join-Path $fpgaRoot "ip\$ipName\$ipName.v"
    Assert-True (Test-Path -LiteralPath $ipPath) "缺少生成 IP：$ipName"
    Assert-Matches (Get-Content -Raw -LiteralPath $ipPath) $generatedIpChecks[$ipName] $ipName
}

foreach ($waveName in 'sin', 'square', 'triangle', 'sawtooth') {
    $mifPath = Join-Path $fpgaRoot "script\$($waveName)_128x14.mif"
    $mif = Get-Content -Raw -LiteralPath $mifPath
    Assert-True ($mif -match 'WIDTH\s*=\s*14\s*;') "$waveName 波形位宽不是 14"
    Assert-True ($mif -match 'DEPTH\s*=\s*128\s*;') "$waveName 波形深度不是 128"
    $samples = [regex]::Matches($mif, '(?m)^\s*(\d+)\s*:\s*(\d+)\s*;')
    Assert-True ($samples.Count -eq 128) "$waveName 波形样本数不是 128"
    foreach ($index in 0..127) {
        Assert-True ([int]$samples[$index].Groups[1].Value -eq $index) "$waveName 波形地址不连续：$index"
        $value = [int]$samples[$index].Groups[2].Value
        Assert-True ($value -ge 0 -and $value -le 16383) "$waveName 波形数据超出 14 位：$value"
    }
}

Write-Host "FPGA 自研模块与示例数据检查通过（$($discoveredModules.Count) 个模块）。"
