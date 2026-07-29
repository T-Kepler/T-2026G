/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, AD9833_SDATA1_Pin|AD9833_SCLK1_Pin|AD9833_FSYNC1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOH, AD9959_PDC_Pin|AD9959_RST_Pin|AD9959_SDIO3_Pin|AD9959_SDIO2_Pin
                          |AD9959_SDIO0_Pin|AD9959_SCK_Pin|AD9959_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, AD9959_SDIO1_Pin|AD9959_P3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOI, AD9959_UP_Pin|AD9959_P2_Pin|AD9959_P0_Pin|AD9959_P1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : AD9833_SDATA1_Pin AD9833_SCLK1_Pin AD9833_FSYNC1_Pin */
  GPIO_InitStruct.Pin = AD9833_SDATA1_Pin|AD9833_SCLK1_Pin|AD9833_FSYNC1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : AD9959 signals on GPIOH
                           PDC J3-5/board50, RST J3-6/board49,
                           SDIO3 J3-7/board52, SDIO2 J3-8/board51,
                           SDIO0 J3-10/board53, SCK J3-11/board56,
                           CS J3-12/board55 */
  GPIO_InitStruct.Pin = AD9959_PDC_Pin|AD9959_RST_Pin|AD9959_SDIO3_Pin|AD9959_SDIO2_Pin
                          |AD9959_SDIO0_Pin|AD9959_SCK_Pin|AD9959_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pins : AD9959_SDIO1_Pin AD9959_P3_Pin
                           SDIO1 J3-9 -> board 54 PG6, P3 J3-16 -> board 59 PG11 */
  GPIO_InitStruct.Pin = AD9959_SDIO1_Pin|AD9959_P3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : AD9959_UP_Pin AD9959_P2_Pin AD9959_P0_Pin AD9959_P1_Pin
                           J3-14 NC lands on board 57 PC7 and is not configured here.
                           UP J3-13 -> board 58 PI0,
                           P2 J3-15 -> board 60 PI2,
                           P0 J3-17 -> board 62 PI4,
                           P1 J3-18 -> board 61 PI5 */
  GPIO_InitStruct.Pin = AD9959_UP_Pin|AD9959_P2_Pin|AD9959_P0_Pin|AD9959_P1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : KEY1_Pin */
  GPIO_InitStruct.Pin = KEY1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : KEY2_Pin KEY3_Pin KEY4_Pin */
  GPIO_InitStruct.Pin = KEY2_Pin|KEY3_Pin|KEY4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
