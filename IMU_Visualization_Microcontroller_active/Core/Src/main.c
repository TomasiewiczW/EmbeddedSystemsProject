/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// definy
#define LSM303_ACC_RESOLUTION 2.0f // max wart przyspieszenia [g]
#define LSM303_MAG_RESOLUTION 4.0f // max wart pola magnetycznego w [G]
#define LSM6DSL_GYR_RESOLUTION 250.0f // max wart dps zyroskopu [dsp]
#define LSM303_MAG_RESOLUTION_SETTING_4_0 0x80 // 100 00000

// Adresy urzadzen
#define LSM303_ACC_ADDRESS (0x19 << 1) // adres akcelerometru: 0011001x OK
#define LSM303_MAG_ADDRESS (0x1E << 1) // adres magnetometru: 0011110x
#define LSM6DSL_GYR_ADDRESS (0x6B << 1) // adres zyroskopu: 01101011x OK

// Rejestry ustawien
#define LSM303_ACC_CTRL_REG1_A 0x20 // rejestr ustawien akcelerometru 1
#define LSM303_MAG_CFG_REG_A_M 0x60 // rejestr ustawien data rate, system mode i temp enable magnetometru
#define LSM6DSL_GYR_CTRL2_G 0x11 // rejestr ustawien data rate zyroskopu
#define LSM6DSL_GYR_CTRL3_C_MULTI_READ 0x12

// Acc
#define LSM303_ACC_X_H_A 0x29
#define LSM303_ACC_X_L_A 0x28
#define LSM303_ACC_Y_H_A 0x2B
#define LSM303_ACC_Y_L_A 0x2A
#define LSM303_ACC_Z_H_A 0x2D
#define LSM303_ACC_Z_L_A 0x2C

// Mag (ZY reversed in memory for some reason according to documentation...)
#define LSM303_MAG_X_H_M 0x69
#define LSM303_MAG_X_L_M 0x68
#define LSM303_MAG_Y_H_M 0x6B
#define LSM303_MAG_Y_L_M 0x6A
#define LSM303_MAG_Z_H_M 0x6D
#define LSM303_MAG_Z_L_M 0x6C

// Gyr
#define LSM6DSL_GYR_X_H_G 0x23
#define LSM6DSL_GYR_X_L_G 0x22
#define LSM6DSL_GYR_Y_H_G 0x25
#define LSM6DSL_GYR_Y_L_G 0x24
#define LSM6DSL_GYR_Z_H_G 0x27
#define LSM6DSL_GYR_Z_L_G 0x26

// Maski bitowe
// CTRL_REG1_A = [ODR3][ODR2][ODR1][ODR0][LPEN][ZEN][YEN][XEN]
#define LSM303_ACC_XYZ_ENABLE 0x07 // 0000 0111
#define LSM303_ACC_100HZ 0x50 //0101 0000
#define LSM303_ACC_X_L_A_MULTI_READ (LSM303_ACC_X_L_A | 0x80)
#define LSM303_MAG_TEMP_DATARATE 0x88 // 1000 1000 TEMP_EN, 10dr=50Hz, continmode


//test WhoAmI
#define UART_BUF_LENGTH 100
volatile uint8_t TxBuf[UART_BUF_LENGTH];
volatile uint8_t RxBuf[UART_BUF_LENGTH];

// Zmienne
uint8_t Data[6]; // Zmienna do bezposredniego odczytu
int16_t acc_x = 0;
int16_t acc_y = 0;
int16_t acc_z = 0; // Zawiera przeksztalcona forme odczytanych danych
int16_t mag_x = 0;
int16_t mag_y = 0;
int16_t mag_z = 0;
int16_t gyr_x = 0;
int16_t gyr_y = 0;
int16_t gyr_z = 0;

float acc_x_g = 0; // zawiera przyspieszenie w osi przeksztalcone na jednostke fizyczna [g]
float acc_y_g = 0;
float acc_z_g = 0;
float mag_x_g = 0;
float mag_y_g = 0;
float mag_z_g = 0;
float gyr_x_dps = 0;
float gyr_y_dps = 0;
float gyr_z_dps = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  // wypelnienie zmiennej konfiguracyjnej odpowiednimi opcjami
  uint8_t acc_settings = LSM303_ACC_XYZ_ENABLE | LSM303_ACC_100HZ;
  uint8_t mag_settings_cfg_a_regm_temp_dr_sm = LSM303_MAG_TEMP_DATARATE;
  uint8_t gyr_settings_multiread = 0x01;
  uint8_t gyr_settings_datarate = 0x30; // 0011 0000 52Hz HF

  // Wpisanie konfiguracji do rejestru akcelerometru
  HAL_I2C_Mem_Write(&hi2c1, LSM303_ACC_ADDRESS, LSM303_ACC_CTRL_REG1_A, 1, &acc_settings, 1, 100);
  // Wpisanie konfiguracji do rejestru magnetometru
  HAL_I2C_Mem_Write(&hi2c1, LSM303_MAG_ADDRESS, LSM303_MAG_CFG_REG_A_M, 1, &mag_settings_cfg_a_regm_temp_dr_sm, 1, 100);
  // Uruchomienie inkrementacji read/write adresu do odczytu wielu wartosci jednoczesnie dla zyroskopu
  HAL_I2C_Mem_Write(&hi2c1, LSM6DSL_GYR_ADDRESS, LSM6DSL_GYR_CTRL3_C_MULTI_READ, 1, &gyr_settings_multiread, 1, 100);
  HAL_I2C_Mem_Write(&hi2c1, LSM6DSL_GYR_ADDRESS, LSM6DSL_GYR_CTRL2_G, 1, &gyr_settings_datarate, 1, 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  	// Pobranie wyzszego bajtu danych osi Z
  	HAL_I2C_Mem_Read(&hi2c1, LSM303_ACC_ADDRESS, LSM303_ACC_X_L_A_MULTI_READ, 1, Data, 6, 100);
	acc_x = ((Data[1] << 8) | Data[0]);
	acc_y = ((Data[3] << 8) | Data[2]);
	acc_z = ((Data[5] << 8) | Data[4]);

	HAL_I2C_Mem_Read(&hi2c1, LSM303_MAG_ADDRESS, LSM303_MAG_X_L_M, 1, Data, 6, 100);
	mag_x = ((Data[1] << 8) | Data[0]);
	mag_y = ((Data[3] << 8) | Data[2]);
	mag_z = ((Data[5] << 8) | Data[4]);

	HAL_I2C_Mem_Read(&hi2c1, LSM6DSL_GYR_ADDRESS, LSM6DSL_GYR_X_L_G, 1, Data, 6, 100); // no multi read in doc..
	gyr_x = ((Data[1] << 8) | Data[0]);
	gyr_y = ((Data[3] << 8) | Data[2]);
	gyr_z = ((Data[5] << 8) | Data[4]);

	// convert to floats
	acc_x_g = ((float) acc_x * LSM303_ACC_RESOLUTION) / (float) INT16_MAX;
	acc_y_g = ((float) acc_y * LSM303_ACC_RESOLUTION) / (float) INT16_MAX;
	acc_z_g = ((float) acc_z * LSM303_ACC_RESOLUTION) / (float) INT16_MAX;
	gyr_x_dps = ((float) gyr_x * LSM6DSL_GYR_RESOLUTION) / (float) INT16_MAX;
	gyr_y_dps = ((float) gyr_y * LSM6DSL_GYR_RESOLUTION) / (float) INT16_MAX;
	gyr_z_dps = ((float) gyr_z * LSM6DSL_GYR_RESOLUTION) / (float) INT16_MAX;
	mag_x_g = ((float) mag_x * LSM303_MAG_RESOLUTION) / (float) INT16_MAX;
	mag_y_g = ((float) mag_y * LSM303_MAG_RESOLUTION) / (float) INT16_MAX;
	mag_z_g = ((float) mag_z * LSM303_MAG_RESOLUTION) / (float) INT16_MAX;

	// prepare floats for sending over serial
	char acc_x_str[10];
	char acc_y_str[10];
	char acc_z_str[10];
	char gyr_x_str[10];
	char gyr_y_str[10];
	char gyr_z_str[10];
	char mag_x_str[10];
	char mag_y_str[10];
	char mag_z_str[10];
	sprintf(acc_x_str, "%f", acc_x_g);
	sprintf(acc_y_str, "%f", acc_y_g);
	sprintf(acc_z_str, "%f", acc_z_g);
	sprintf(gyr_x_str, "%f", gyr_x_dps);
	sprintf(gyr_y_str, "%f", gyr_y_dps);
	sprintf(gyr_z_str, "%f", gyr_z_dps);
	sprintf(mag_x_str, "%f", mag_x_g);
	sprintf(mag_y_str, "%f", mag_y_g);
	sprintf(mag_z_str, "%f", mag_z_g);

	// Madgwick AHRS - consider tweaking sample freq/delta time in implementation
	MadgwickAHRS(acc_x_g, acc_y_g, acc_z_g, gyr_x_dps, gyr_y_dps, gyr_z_dps, mag_x_g, mag_y_g, mag_z_g);

	// prepare quaternion floats for sending over serial
	char q0_str[10];
	char q1_str[10];
	char q2_str[10];
	char q3_str[10];
	sprintf(q0_str, "%f", q0);
	sprintf(q1_str, "%f", q1);
	sprintf(q2_str, "%f", q2);
	sprintf(q3_str, "%f", q3);


	  uint8_t TxSize;
	  /*TxSize = snprintf(TxBuf,sizeof(TxBuf),"ACC_GYR_MAG RAW OUTPUT\r\n");
	  HAL_UART_Transmit(&huart2, TxBuf, TxSize,100);
	  TxSize = snprintf(TxBuf,sizeof(TxBuf),"LSM303: AccX= %d, AccY= %d, AccZ= %d\r\n",acc_x, acc_y, acc_z);
	  HAL_UART_Transmit(&huart2, TxBuf, TxSize,100);

	  TxSize = snprintf(TxBuf,sizeof(TxBuf),"LSM6DSL: GyrX= %d, GyrY= %d, GyrZ= %d\r\n",gyr_x, gyr_y, gyr_z);
	  HAL_UART_Transmit(&huart2, TxBuf, TxSize,100);

	  TxSize = snprintf(TxBuf,sizeof(TxBuf),"LSM303: MagX= %d, MagY= %d, MagZ= %d\r\n",mag_x, mag_y, mag_z);
	  HAL_UART_Transmit(&huart2, TxBuf, TxSize,100);
	  HAL_Delay(2000);


	  TxSize = snprintf(TxBuf,sizeof(TxBuf),"ACC_GYR_MAG UNIT OUTPUT [g][dps][G]\r\n");
	  HAL_UART_Transmit(&huart2, TxBuf, TxSize,100);
	  TxSize = snprintf(TxBuf,sizeof(TxBuf),"LSM303: AccX#AccY#AccZ=%s#%s#%s\r\n", acc_x_str, acc_y_str, acc_z_str);
	  HAL_UART_Transmit(&huart2, TxBuf, TxSize,100);

	  TxSize = snprintf(TxBuf,sizeof(TxBuf),"LSM6DSL: GyrX#GyrY#GyrZ=%s#%s#%s\r\n", gyr_x_str, gyr_y_str, gyr_z_str);
	  HAL_UART_Transmit(&huart2, TxBuf, TxSize,100);

	  TxSize = snprintf(TxBuf,sizeof(TxBuf),"LSM303: MagX#MagY#MagZ %s#%s#%s\r\n", mag_x_str, mag_y_str, mag_z_str);
	  HAL_UART_Transmit(&huart2, TxBuf, TxSize,100);
	  HAL_Delay(2000);


	  TxSize = snprintf(TxBuf,sizeof(TxBuf),"QUATERNION OUTPUT q0q1q2q3\r\n");
	  HAL_UART_Transmit(&huart2, TxBuf, TxSize,100); */
	  TxSize = snprintf(TxBuf,sizeof(TxBuf),"%s#%s#%s#%s\r\n", q0_str, q1_str, q2_str, q3_str);
	  HAL_UART_Transmit(&huart2, TxBuf, TxSize,100);

	  HAL_Delay(2000);


  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

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

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
