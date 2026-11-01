/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "i2c.h"
#include "rtc.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "OLED.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  OLED_Clear();
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  RTC_AlarmTypeDef sAlarm = {0};
  uint8_t set_mode = 0; 

// 2. 初始化闹钟（写在 while 外面）
  RTC_TimeTypeDef ResetTime = {0};
  RTC_DateTypeDef ResetDate = {0};
  ResetTime.Hours = 12; ResetTime.Minutes = 0; ResetTime.Seconds = 0;
  ResetDate.Year = 26; ResetDate.Month = 10; ResetDate.Date = 24; ResetDate.WeekDay = RTC_WEEKDAY_SATURDAY;
  HAL_RTC_SetTime(&hrtc, &ResetTime, RTC_FORMAT_BIN);
  HAL_RTC_SetDate(&hrtc, &ResetDate, RTC_FORMAT_BIN);
  
  // 👇 补上闹钟的初始化（默认设为 12:30:00）
  sAlarm.AlarmTime.Hours = 12;
  sAlarm.AlarmTime.Minutes = 30;
  sAlarm.AlarmTime.Seconds = 0;
  sAlarm.Alarm = RTC_ALARM_A; 
  //HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN); // 必须写这句，闹钟才会生效

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET); 
  while (1)
  {
    // -----------------------------------------
	// 1. 读取并显示时间
	// -----------------------------------------
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	// 显示日期（例如 2026-10-24）
	OLED_ShowNum(1, 1, 2000 + sDate.Year, 4); 
	OLED_ShowString(1, 5, "-");
	OLED_ShowNum(1, 6, sDate.Month, 2);
	OLED_ShowString(1, 8, "-");
	OLED_ShowNum(1, 9, sDate.Date, 2);

	// 显示时间（例如 12:00:00）
	OLED_ShowNum(2, 1, sTime.Hours, 2);
	OLED_ShowString(2, 3, ":");
	OLED_ShowNum(2, 4, sTime.Minutes, 2);
	OLED_ShowString(2, 6, ":");
	OLED_ShowNum(2, 7, sTime.Seconds, 2);

	// 显示当前模式（0-4），方便看状态
	OLED_ShowString(3, 1, "Mode:");
	OLED_ShowNum(3, 6, set_mode, 1);
	OLED_ShowString(3, 8, "    "); 
    // 👇 补上闹钟时间的显示（显示在第4行）
    OLED_ShowString(4, 1, "Alarm:");
    OLED_ShowNum(4, 7, sAlarm.AlarmTime.Hours, 2);
    OLED_ShowString(4, 9, ":");
    OLED_ShowNum(4, 10, sAlarm.AlarmTime.Minutes, 2);
	// 👇 新增：暴力比对时间，时间一到就让蜂鸣器响！
	if (set_mode == 0) // 只有在正常走时模式下才检测
	{
		// 如果当前的 时、分、秒 刚好等于闹钟的 时、分、秒
		if (sTime.Hours == sAlarm.AlarmTime.Hours && 
			sTime.Minutes == sAlarm.AlarmTime.Minutes && 
			sTime.Seconds == sAlarm.AlarmTime.Seconds) 
		{
			// 蜂鸣器响（PB15）
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
			HAL_Delay(2000); // 响 2 秒
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
			
			// 🚨 极其关键：响完之后，把闹钟小时改成 99，防止这一分钟内反复触发！
			sAlarm.AlarmTime.Hours = 99; 
			// 如果想让它明天同一时间继续响，可以改成 sAlarm.AlarmTime.Minutes = 99;
		}
	}
	// -----------------------------------------
	// 2. 按键检测：PB11 (模式切换)
	// -----------------------------------------
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_RESET) 
	{
		HAL_Delay(20); // 消抖
		if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_RESET) 
		{
			set_mode++;
			if (set_mode > 4) set_mode = 0; // 0->1->2->3->4->0 循环
			while(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_RESET); // 等松手
		}
	}

	// -----------------------------------------
	// 3. 按键检测：PA0 (加号键)
	// -----------------------------------------
	if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) 
	{
		HAL_Delay(20);
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) 
		{
			// 调时（模式1）
			if (set_mode == 1) { 
				sTime.Hours++; if(sTime.Hours > 23) sTime.Hours = 0; 
				HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN); 
			}
			// 调分（模式2）
			if (set_mode == 2) { 
				sTime.Minutes++; if(sTime.Minutes > 59) sTime.Minutes = 0; 
				HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN); 
			}
			// 调闹钟时（模式3）
			if (set_mode == 3) { 
				sAlarm.AlarmTime.Hours++; if(sAlarm.AlarmTime.Hours > 23) sAlarm.AlarmTime.Hours = 0; 
				//HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN); 
			}
			// 调闹钟分（模式4）
			if (set_mode == 4) { 
				sAlarm.AlarmTime.Minutes++; if(sAlarm.AlarmTime.Minutes > 59) sAlarm.AlarmTime.Minutes = 0; 
				//HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN); 
			}
			while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET);
		}
	}

	// -----------------------------------------
	// 4. 按键检测：PA2(减号键)
	// -----------------------------------------
	if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET) 
	{
		HAL_Delay(20);
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET) 
		{
			// 减时
			if (set_mode == 1) { 
				if(sTime.Hours > 0) sTime.Hours--; else sTime.Hours = 23; 
				HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN); 
			}
			// 减分
			if (set_mode == 2) { 
				if(sTime.Minutes > 0) sTime.Minutes--; else sTime.Minutes = 59; 
				HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN); 
			}
			// 减闹钟时
			if (set_mode == 3) { 
				if(sAlarm.AlarmTime.Hours > 0) sAlarm.AlarmTime.Hours--; else sAlarm.AlarmTime.Hours = 23; 
				//HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN); 
			}
			// 减闹钟分
			if (set_mode == 4) { 
				if(sAlarm.AlarmTime.Minutes > 0) sAlarm.AlarmTime.Minutes--; else sAlarm.AlarmTime.Minutes = 59; 
				//HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN); 
			}
			while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET);
		}
	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
