$ErrorActionPreference = 'Stop'

# 兼容旧入口；实际测试统一放在 test 目录。
& (Join-Path $PSScriptRoot '..\test\test_pure_verilog.ps1')
