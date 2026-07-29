# Copyright (C) 2018  Intel Corporation. All rights reserved.
# Your use of Intel Corporation's design tools, logic functions 
# and other software and tools, and its AMPP partner logic 
# functions, and any output files from any of the foregoing 
# (including device programming or simulation files), and any 
# associated documentation or information are expressly subject 
# to the terms and conditions of the Intel Program License 
# Subscription Agreement, the Intel Quartus Prime License Agreement,
# the Intel FPGA IP License Agreement, or other applicable license
# agreement, including, without limitation, that your use is for
# the sole purpose of programming logic devices manufactured by
# Intel and sold by Intel or its authorized distributors.  Please
# refer to the applicable agreement for further details.

# Quartus Prime Version 18.1.0 Build 625 09/12/2018 SJ Standard Edition
# File: E:\competition\EE\Electronic\EE_OBJECT\code\zuolan_signal_fpga_stm32\zuolan_FPGA\script\ZUOLAN_FPGA_OBJECT.tcl
# Generated on: Sat Jul 19 00:38:43 2025

package require ::quartus::project

set_location_assignment PIN_E1 -to CLK
set_location_assignment PIN_E16 -to RST
set_location_assignment PIN_F1 -to FPGA_CS_NEL
set_location_assignment PIN_F5 -to FPGA_NL_NADV
set_location_assignment PIN_J1 -to FPGA_RD_NOE
set_location_assignment PIN_G2 -to FPGA_WR_NWE
set_location_assignment PIN_N1 -to FPGA_DB[0]
set_location_assignment PIN_N2 -to FPGA_DB[1]
set_location_assignment PIN_P1 -to FPGA_DB[2]
set_location_assignment PIN_J6 -to FPGA_DB[3]
set_location_assignment PIN_P2 -to FPGA_DB[4]
set_location_assignment PIN_R1 -to FPGA_DB[5]
set_location_assignment PIN_N3 -to FPGA_DB[6]
set_location_assignment PIN_P3 -to FPGA_DB[7]
set_location_assignment PIN_T2 -to FPGA_DB[8]
set_location_assignment PIN_R3 -to FPGA_DB[9]
set_location_assignment PIN_T3 -to FPGA_DB[10]
set_location_assignment PIN_R4 -to FPGA_DB[11]
set_location_assignment PIN_T4 -to FPGA_DB[12]
set_location_assignment PIN_R5 -to FPGA_DB[13]
set_location_assignment PIN_N5 -to FPGA_DB[14]
set_location_assignment PIN_L6 -to FPGA_DB[15]
set_location_assignment PIN_A7 -to DA1_OUT[13]
set_location_assignment PIN_D8 -to DA1_OUT[12]
set_location_assignment PIN_B7 -to DA1_OUT[11]
set_location_assignment PIN_C8 -to DA1_OUT[10]
set_location_assignment PIN_A8 -to DA1_OUT[9]
set_location_assignment PIN_F8 -to DA1_OUT[8]
set_location_assignment PIN_B8 -to DA1_OUT[7]
set_location_assignment PIN_F9 -to DA1_OUT[6]
set_location_assignment PIN_A9 -to DA1_OUT[5]
set_location_assignment PIN_E9 -to DA1_OUT[4]
set_location_assignment PIN_B9 -to DA1_OUT[3]
set_location_assignment PIN_D9 -to DA1_OUT[2]
set_location_assignment PIN_A10 -to DA1_OUT[1]
set_location_assignment PIN_C9 -to DA1_OUT[0]
set_location_assignment PIN_B10 -to DA1_OUTCLK
set_location_assignment PIN_B11 -to DA2_OUT[13]
set_location_assignment PIN_C11 -to DA2_OUT[12]
set_location_assignment PIN_A12 -to DA2_OUT[11]
set_location_assignment PIN_E11 -to DA2_OUT[10]
set_location_assignment PIN_B12 -to DA2_OUT[9]
set_location_assignment PIN_D12 -to DA2_OUT[8]
set_location_assignment PIN_A13 -to DA2_OUT[7]
set_location_assignment PIN_F11 -to DA2_OUT[6]
set_location_assignment PIN_B13 -to DA2_OUT[5]
set_location_assignment PIN_C14 -to DA2_OUT[4]
set_location_assignment PIN_A14 -to DA2_OUT[3]
set_location_assignment PIN_D14 -to DA2_OUT[2]
set_location_assignment PIN_B14 -to DA2_OUT[1]
set_location_assignment PIN_F10 -to DA2_OUT[0]
set_location_assignment PIN_A11 -to DA2_OUTCLK
set_location_assignment PIN_G15 -to AD1_INPUT[11]
set_location_assignment PIN_F13 -to AD1_INPUT[10]
set_location_assignment PIN_J15 -to AD1_INPUT[9]
set_location_assignment PIN_F14 -to AD1_INPUT[8]
set_location_assignment PIN_J16 -to AD1_INPUT[7]
set_location_assignment PIN_G11 -to AD1_INPUT[6]
set_location_assignment PIN_K16 -to AD1_INPUT[5]
set_location_assignment PIN_J11 -to AD1_INPUT[4]
set_location_assignment PIN_L16 -to AD1_INPUT[3]
set_location_assignment PIN_K11 -to AD1_INPUT[2]
set_location_assignment PIN_L15 -to AD1_INPUT[1]
set_location_assignment PIN_J12 -to AD1_INPUT[0]
set_location_assignment PIN_C15 -to AD1_OUTCLK
set_location_assignment PIN_J13 -to AD2_OUTCLK
set_location_assignment PIN_N15 -to AD2_INPUT[11]
set_location_assignment PIN_J14 -to AD2_INPUT[10]
set_location_assignment PIN_P16 -to AD2_INPUT[9]
set_location_assignment PIN_K12 -to AD2_INPUT[8]
set_location_assignment PIN_P15 -to AD2_INPUT[7]
set_location_assignment PIN_K15 -to AD2_INPUT[6]
set_location_assignment PIN_R16 -to AD2_INPUT[5]
set_location_assignment PIN_L12 -to AD2_INPUT[4]
set_location_assignment PIN_L14 -to AD2_INPUT[3]
set_location_assignment PIN_L13 -to AD2_INPUT[2]
set_location_assignment PIN_M12 -to AD2_INPUT[1]
set_location_assignment PIN_N14 -to AD2_INPUT[0]
set_location_assignment PIN_D15 -to AD1_INPUT_CLK
set_location_assignment PIN_D16 -to AD2_INPUT_CLK
