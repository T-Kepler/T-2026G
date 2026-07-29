/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define AD9833_SDATA1_Pin GPIO_PIN_6
#define AD9833_SDATA1_GPIO_Port GPIOF
#define AD9833_SCLK1_Pin GPIO_PIN_8
#define AD9833_SCLK1_GPIO_Port GPIOF
#define AD9833_FSYNC1_Pin GPIO_PIN_9
#define AD9833_FSYNC1_GPIO_Port GPIOF

/* AD9959 module J3 -> main board header mapping for the 14-wire ribbon cable.
 * J3-1/2/3/4 are left unconnected. The two header rows are swapped, so
 * J3-5 starts at main board header 50 and J3-6 starts at header 49.
 *
 *   AD9959 J3 signal      Main board header
 *   J3-5  PWR_DWN  ->     50 PH9
 *   J3-6  RESET    ->     49 PH10
 *   J3-7  SDIO3    ->     52 PH11
 *   J3-8  SDIO2    ->     51 PH12
 *   J3-9  SDIO1    ->     54 PG6
 *   J3-10 SDIO0    ->     53 PH13
 *   J3-11 SCLK     ->     56 PH14
 *   J3-12 CSB      ->     55 PH15
 *   J3-13 IO_UP    ->     58 PI0
 *   J3-14 NC       ->     57 PC7, left as input/reserved
 *   J3-15 P2       ->     60 PI2
 *   J3-16 P3       ->     59 PG11
 *   J3-17 P0       ->     62 PI4
 *   J3-18 P1       ->     61 PI5
 */
#define AD9959_PDC_Pin GPIO_PIN_9       /* J3-5  PWR_DWN -> board 50 PH9 */
#define AD9959_PDC_GPIO_Port GPIOH
#define AD9959_RST_Pin GPIO_PIN_10      /* J3-6  RESET   -> board 49 PH10 */
#define AD9959_RST_GPIO_Port GPIOH
#define AD9959_SDIO3_Pin GPIO_PIN_11    /* J3-7  SDIO3   -> board 52 PH11 */
#define AD9959_SDIO3_GPIO_Port GPIOH
#define AD9959_SDIO2_Pin GPIO_PIN_12    /* J3-8  SDIO2   -> board 51 PH12 */
#define AD9959_SDIO2_GPIO_Port GPIOH
#define AD9959_SDIO1_Pin GPIO_PIN_6     /* J3-9  SDIO1   -> board 54 PG6 */
#define AD9959_SDIO1_GPIO_Port GPIOG
#define AD9959_SDIO0_Pin GPIO_PIN_13    /* J3-10 SDIO0   -> board 53 PH13 */
#define AD9959_SDIO0_GPIO_Port GPIOH
#define AD9959_SCK_Pin GPIO_PIN_14      /* J3-11 SCLK    -> board 56 PH14 */
#define AD9959_SCK_GPIO_Port GPIOH
#define AD9959_CS_Pin GPIO_PIN_15       /* J3-12 CSB     -> board 55 PH15 */
#define AD9959_CS_GPIO_Port GPIOH
#define AD9959_UP_Pin GPIO_PIN_0        /* J3-13 IO_UP   -> board 58 PI0 */
#define AD9959_UP_GPIO_Port GPIOI
#define AD9959_P2_Pin GPIO_PIN_2        /* J3-15 P2      -> board 60 PI2 */
#define AD9959_P2_GPIO_Port GPIOI
#define AD9959_P3_Pin GPIO_PIN_11       /* J3-16 P3      -> board 59 PG11 */
#define AD9959_P3_GPIO_Port GPIOG
#define AD9959_P0_Pin GPIO_PIN_4        /* J3-17 P0      -> board 62 PI4 */
#define AD9959_P0_GPIO_Port GPIOI
#define AD9959_P1_Pin GPIO_PIN_5        /* J3-18 P1      -> board 61 PI5 */
#define AD9959_P1_GPIO_Port GPIOI
#define KEY1_Pin GPIO_PIN_6
#define KEY1_GPIO_Port GPIOD
#define KEY2_Pin GPIO_PIN_4
#define KEY2_GPIO_Port GPIOB
#define KEY3_Pin GPIO_PIN_6
#define KEY3_GPIO_Port GPIOB
#define KEY4_Pin GPIO_PIN_9
#define KEY4_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
