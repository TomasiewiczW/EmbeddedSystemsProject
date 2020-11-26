/**
  ******************************************************************************
  * File Name          : stmicroelectronics_x-cube-mems1_7_2_0.c
  * Description        : This file provides code for the configuration
  *                      of the STMicroelectronics.X-CUBE-MEMS1.7.2.0 instances.
  ******************************************************************************
  *
  * COPYRIGHT 2020 STMicroelectronics
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  ******************************************************************************
  */

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "app_x-cube-mems1.h"
#include "main.h"
#include <stdio.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_nucleo.h"
#include "com.h"
#include "demo_serial.h"
#include "motion_fx_manager.h"
#include "bsp_ip_conf.h"
#include "fw_version.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ALGO_FREQ    100U /* Algorithm frequency [Hz] */

#define ALGO_PERIOD  10   /* Algorithm period [ms] */

#define MOTION_FX_ENGINE_DELTATIME  0.01f

#define FROM_MG_TO_G         0.001f
#define FROM_G_TO_MG         1000.0f
#define FROM_MDPS_TO_DPS     0.001f
#define FROM_DPS_TO_MDPS     1000.0f
#define FROM_MGAUSS_TO_UT50  (0.1f/50.0f)
#define FROM_UT50_TO_MGAUSS  500.0f

#define DWT_LAR_KEY  0xC5ACCE55 /* DWT register unlock key */

/* Public variables ----------------------------------------------------------*/
volatile uint8_t DataLoggerActive = 0;
volatile uint32_t SensorsEnabled = 0;
uint8_t Enabled6X = 0;

char lib_version[35];
int lib_version_len;

/* Extern variables ----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static volatile uint8_t SensorReadRequest = 0;
static volatile uint8_t MagCalRequest = 0;
static MOTION_SENSOR_Axes_t AccValue;
static MOTION_SENSOR_Axes_t GyrValue;
static MOTION_SENSOR_Axes_t MagValue;
static MOTION_SENSOR_Axes_t MagOffset;

static uint32_t MagTimeStamp = 0;

static uint8_t MagCalStatus = 0;

/* Private function prototypes -----------------------------------------------*/
static void MX_DataLogFusion_Init(void);
static void MX_DataLogFusion_Process(void);

static void Init_Sensors(void);
static void RTC_Handler(TMsg *Msg);
static void FX_Data_Handler(TMsg *Msg);
static void Accelero_Sensor_Handler(TMsg *Msg);
static void Gyro_Sensor_Handler(TMsg *Msg);
static void Magneto_Sensor_Handler(TMsg *Msg);
static void Pressure_Sensor_Handler(TMsg *Msg);
static void Humidity_Sensor_Handler(TMsg *Msg);
static void Temperature_Sensor_Handler(TMsg *Msg);
static void TIM_Config(uint32_t Freq);
static void DWT_Init(void);
static void DWT_Start(void);
static uint32_t DWT_Stop(void);

void MX_MEMS_Init(void)
{
  /* USER CODE BEGIN SV */

  /* USER CODE END SV */

  /* USER CODE BEGIN MEMS_Init_PreTreatment */

  /* USER CODE END MEMS_Init_PreTreatment */

  /* Initialize the peripherals and the MEMS components */

  MX_DataLogFusion_Init();

  /* USER CODE BEGIN MEMS_Init_PostTreatment */

  /* USER CODE END MEMS_Init_PostTreatment */
}

/*
 * LM background task
 */
void MX_MEMS_Process(void)
{
  /* USER CODE BEGIN MEMS_Process_PreTreatment */

  /* USER CODE END MEMS_Process_PreTreatment */

  MX_DataLogFusion_Process();

  /* USER CODE BEGIN MEMS_Process_PostTreatment */

  /* USER CODE END MEMS_Process_PostTreatment */
}

/* Exported functions --------------------------------------------------------*/
/**
 * @brief  BSP Push Button callback
 * @param  Button Specifies the pin connected EXTI line
 * @retval None
 */
void BSP_PB_Callback(Button_TypeDef Button)
{
  if (Button == BUTTON_USER)
  {
    if (BSP_PB_GetState(BUTTON_USER) == (uint32_t)GPIO_PIN_RESET)
    {
      MagCalRequest = 1;
    }
  }
}

/**
 * @brief  Period elapsed callback
 * @param  htim pointer to a TIM_HandleTypeDef structure that contains
 *              the configuration information for TIM module
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == BSP_IP_TIM_Handle.Instance)
  {
    SensorReadRequest = 1;
  }
}

/**
 * @brief  Configures the current date
 * @param  y the year value to be set
 * @param  m the month value to be set
 * @param  d the day value to be set
 * @param  dw the day-week value to be set
 * @retval None
 */
void RTC_DateRegulate(uint8_t y, uint8_t m, uint8_t d, uint8_t dw)
{
  RTC_DateTypeDef sdatestructure;

  sdatestructure.Year    = y;
  sdatestructure.Month   = m;
  sdatestructure.Date    = d;
  sdatestructure.WeekDay = dw;

  if (HAL_RTC_SetDate(&hrtc, &sdatestructure, FORMAT_BIN) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

/**
 * @brief  Configures the current time
 * @param  hh the hour value to be set
 * @param  mm the minute value to be set
 * @param  ss the second value to be set
 * @retval None
 */
void RTC_TimeRegulate(uint8_t hh, uint8_t mm, uint8_t ss)
{
  RTC_TimeTypeDef stimestructure;

  stimestructure.Hours          = hh;
  stimestructure.Minutes        = mm;
  stimestructure.Seconds        = ss;
  stimestructure.SubSeconds     = 0;
  stimestructure.TimeFormat     = RTC_HOURFORMAT12_AM;
  stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

  if (HAL_RTC_SetTime(&hrtc, &stimestructure, FORMAT_BIN) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

/**
 * @brief  Gets Presentation string
 * @param  PresentationString the Presentation string
 * @param  Length the length of Presentation string
 * @retval None
 */
void Get_PresentationString(char *PresentationString, uint32_t *Length)
{
  char fw_id[] = {"4"}; /* FW ID for Unicleo-GUI */
  const uint8_t string_pointer_shift = strlen("ST MotionXX v"); /* Shift string pointer by this amount */
  char *lib_version_num;
  char lib_version_string[64];
  int lib_version_len = 0;

  MotionFX_manager_get_version(lib_version_string, &lib_version_len);

  /* Shorten library version string (e.g.: ST MotionXX v1.0.0) to contain version number only (e.g.: 1.0.0) */
  if (lib_version_len > string_pointer_shift)
  {
    lib_version_num = lib_version_string + string_pointer_shift;
  }
  else
  {
    lib_version_num = lib_version_string;
  }

  *Length = snprintf(PresentationString, 64, "MEMS shield demo,%s,%s,%s,%s", fw_id, FW_VERSION, lib_version_num, BOARD_NAME);
}

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initialize the DataLogFusion application
  * @retval None
  */
static void MX_DataLogFusion_Init(void)
{
  float ans_float;

  /* Initialize LED */
  BSP_LED_Init(LED2);

  /* Initialize push button */
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize Virtual COM Port */
  BSP_COM_Init(COM1);

  /* Initialize Timer */
  BSP_IP_TIM_Init();

  /* Configure Timer to run with desired algorithm frequency */
  TIM_Config(ALGO_FREQ);

  /* Initialize (disabled) Sensors */
  Init_Sensors();

  /* Sensor Fusion API initialization function */
  MotionFX_manager_init();

  /* OPTIONAL */
  /* Get library version */
  MotionFX_manager_get_version(lib_version, &lib_version_len);

  /* LED Blink */
  DWT_Init();

  BSP_LED_On(LED2);
  HAL_Delay(500);
  BSP_LED_Off(LED2);

  /* Enable magnetometer calibration */
  MotionFX_manager_MagCal_start(ALGO_PERIOD);

  /* Test if calibration data are available */
  MFX_MagCal_output_t mag_cal_test;
  MotionFX_MagCal_getParams(&mag_cal_test);

  /* If calibration data are available load HI coeficients */
  if (mag_cal_test.cal_quality == MFX_MAGCALGOOD)
  {
    ans_float = (mag_cal_test.hi_bias[0] * FROM_UT50_TO_MGAUSS);
    MagOffset.x = (int32_t)ans_float;
    ans_float = (mag_cal_test.hi_bias[1] * FROM_UT50_TO_MGAUSS);
    MagOffset.y = (int32_t)ans_float;
    ans_float = (mag_cal_test.hi_bias[2] * FROM_UT50_TO_MGAUSS);
    MagOffset.z = (int32_t)ans_float;

    MagCalStatus = 1;
  }

  /* Start receiving messages via DMA */
  UART_StartReceiveMsg();
}

/**
  * @brief  Process of the DataLogFusion application
  * @retval None
  */
static void MX_DataLogFusion_Process(void)
{
  static TMsg msg_dat;
  static TMsg msg_cmd;

  if (UART_ReceivedMSG((TMsg *)&msg_cmd) != 1)
  {
    if (msg_cmd.Data[0] == DEV_ADDR)
    {
      (void)HandleMSG((TMsg *)&msg_cmd);
    }
  }

  if (MagCalRequest == 1U)
  {
    MagCalRequest = 0;

    /* Reset magnetometer calibration value*/
    MagCalStatus = 0;
    MagOffset.x = 0;
    MagOffset.y = 0;
    MagOffset.z = 0;

    /* Enable magnetometer calibration */
    MotionFX_manager_MagCal_start(ALGO_PERIOD);
  }

  if (SensorReadRequest == 1U)
  {
    SensorReadRequest = 0;

    /* Acquire data from enabled sensors and fill Msg stream */
    RTC_Handler(&msg_dat);
    Accelero_Sensor_Handler(&msg_dat);
    Gyro_Sensor_Handler(&msg_dat);
    Magneto_Sensor_Handler(&msg_dat);
    Humidity_Sensor_Handler(&msg_dat);
    Temperature_Sensor_Handler(&msg_dat);
    Pressure_Sensor_Handler(&msg_dat);

    /* Sensor Fusion specific part */
    FX_Data_Handler(&msg_dat);

    /* Send data stream */
    INIT_STREAMING_HEADER(&msg_dat);
    msg_dat.Len = STREAMING_MSG_LENGTH;
    UART_SendMsg(&msg_dat);
  }
}

/**
 * @brief  Initialize all sensors
 * @param  None
 * @retval None
 */
static void Init_Sensors(void)
{
  BSP_SENSOR_ACC_Init();
  BSP_SENSOR_ACC_Disable();

  BSP_SENSOR_GYR_Init();
  BSP_SENSOR_GYR_Disable();

  BSP_SENSOR_MAG_Init();
  BSP_SENSOR_MAG_Disable();

  BSP_SENSOR_PRESS_Init();
  BSP_SENSOR_PRESS_Disable();

  BSP_SENSOR_TEMP_Init();
  BSP_SENSOR_TEMP_Disable();

  BSP_SENSOR_HUM_Init();
  BSP_SENSOR_HUM_Disable();
}

/**
 * @brief  Handles the time+date getting/sending
 * @param  Msg the time+date part of the stream
 * @retval None
 */
static void RTC_Handler(TMsg *Msg)
{
  uint8_t sub_sec = 0;
  RTC_DateTypeDef sdatestructureget;
  RTC_TimeTypeDef stimestructure;
  uint32_t ans_uint32;
  int32_t ans_int32;
  uint32_t RtcSynchPrediv = hrtc.Init.SynchPrediv;

  (void)HAL_RTC_GetTime(&hrtc, &stimestructure, FORMAT_BIN);
  (void)HAL_RTC_GetDate(&hrtc, &sdatestructureget, FORMAT_BIN);

  /* To be MISRA C-2012 compliant the original calculation:
     sub_sec = ((((((int)RtcSynchPrediv) - ((int)stimestructure.SubSeconds)) * 100) / (RtcSynchPrediv + 1)) & 0xFF);
     has been split to separate expressions */
  ans_int32 = (RtcSynchPrediv - (int32_t)stimestructure.SubSeconds) * 100;
  ans_int32 /= RtcSynchPrediv + 1;
  ans_uint32 = (uint32_t)ans_int32 & 0xFFU;
  sub_sec = (uint8_t)ans_uint32;

  Msg->Data[3] = (uint8_t)stimestructure.Hours;
  Msg->Data[4] = (uint8_t)stimestructure.Minutes;
  Msg->Data[5] = (uint8_t)stimestructure.Seconds;
  Msg->Data[6] = sub_sec;
}

/**
 * @brief  Sensor Fusion data handler
 * @param  Msg the Sensor Fusion data part of the stream
 * @retval None
 */
static void FX_Data_Handler(TMsg *Msg)
{
  uint32_t elapsed_time_us = 0U;
  MFX_input_t data_in;
  MFX_input_t *pdata_in = &data_in;
  MFX_output_t data_out;
  MFX_output_t *pdata_out = &data_out;

  if ((SensorsEnabled & ACCELEROMETER_SENSOR) == ACCELEROMETER_SENSOR)
  {
    if ((SensorsEnabled & GYROSCOPE_SENSOR) == GYROSCOPE_SENSOR)
    {
      if ((SensorsEnabled & MAGNETIC_SENSOR) == MAGNETIC_SENSOR)
      {
        /* Convert angular velocity from [mdps] to [dps] */
        data_in.gyro[0] = (float)GyrValue.x * FROM_MDPS_TO_DPS;
        data_in.gyro[1] = (float)GyrValue.y * FROM_MDPS_TO_DPS;
        data_in.gyro[2] = (float)GyrValue.z * FROM_MDPS_TO_DPS;

        /* Convert acceleration from [mg] to [g] */
        data_in.acc[0] = (float)AccValue.x * FROM_MG_TO_G;
        data_in.acc[1] = (float)AccValue.y * FROM_MG_TO_G;
        data_in.acc[2] = (float)AccValue.z * FROM_MG_TO_G;

        /* Convert magnetic field intensity from [mGauss] to [uT / 50] */
        data_in.mag[0] = (float)MagValue.x * FROM_MGAUSS_TO_UT50;
        data_in.mag[1] = (float)MagValue.y * FROM_MGAUSS_TO_UT50;
        data_in.mag[2] = (float)MagValue.z * FROM_MGAUSS_TO_UT50;

        /* Run Sensor Fusion algorithm */
        BSP_LED_On(LED2);
        DWT_Start();
        // Commented out for Madgwick implementation instead
        //MotionFX_manager_run(pdata_in, pdata_out, MOTION_FX_ENGINE_DELTATIME);
        MadgwickAHRS(pdata_in, pdata_out, MOTION_FX_ENGINE_DELTATIME);
        elapsed_time_us = DWT_Stop();
        BSP_LED_Off(LED2);

        if (Enabled6X == 1U)
        {
          (void)memcpy(&Msg->Data[55], (void *)pdata_out->quaternion_6X, 4U * sizeof(float));
          (void)memcpy(&Msg->Data[71], (void *)pdata_out->rotation_6X, 3U * sizeof(float));
          (void)memcpy(&Msg->Data[83], (void *)pdata_out->gravity_6X, 3U * sizeof(float));
          (void)memcpy(&Msg->Data[95], (void *)pdata_out->linear_acceleration_6X, 3U * sizeof(float));

          (void)memcpy(&Msg->Data[107], (void *) & (pdata_out->heading_6X), sizeof(float));
          (void)memcpy(&Msg->Data[111], (void *) & (pdata_out->headingErr_6X), sizeof(float));
        }
        else
        {
          (void)memcpy(&Msg->Data[55], (void *)pdata_out->quaternion_9X, 4U * sizeof(float));
          (void)memcpy(&Msg->Data[71], (void *)pdata_out->rotation_9X, 3U * sizeof(float));
          (void)memcpy(&Msg->Data[83], (void *)pdata_out->gravity_9X, 3U * sizeof(float));
          (void)memcpy(&Msg->Data[95], (void *)pdata_out->linear_acceleration_9X, 3U * sizeof(float));

          (void)memcpy(&Msg->Data[107], (void *) & (pdata_out->heading_9X), sizeof(float));
          (void)memcpy(&Msg->Data[111], (void *) & (pdata_out->headingErr_9X), sizeof(float));
        }

        Serialize_s32(&Msg->Data[115], (int32_t)elapsed_time_us, 4);
      }
    }
  }
}

/**
 * @brief  Handles the ACC axes data getting/sending
 * @param  Msg the ACC part of the stream
 * @retval None
 */
static void Accelero_Sensor_Handler(TMsg *Msg)
{
  if ((SensorsEnabled & ACCELEROMETER_SENSOR) == ACCELEROMETER_SENSOR)
  {
    BSP_SENSOR_ACC_GetAxes(&AccValue);
    Serialize_s32(&Msg->Data[19], (int32_t)AccValue.x, 4);
    Serialize_s32(&Msg->Data[23], (int32_t)AccValue.y, 4);
    Serialize_s32(&Msg->Data[27], (int32_t)AccValue.z, 4);
  }
}

/**
 * @brief  Handles the GYR axes data getting/sending
 * @param  Msg the GYR part of the stream
 * @retval None
 */
static void Gyro_Sensor_Handler(TMsg *Msg)
{
  if ((SensorsEnabled & GYROSCOPE_SENSOR) == GYROSCOPE_SENSOR)
  {
    BSP_SENSOR_GYR_GetAxes(&GyrValue);
    Serialize_s32(&Msg->Data[31], GyrValue.x, 4);
    Serialize_s32(&Msg->Data[35], GyrValue.y, 4);
    Serialize_s32(&Msg->Data[39], GyrValue.z, 4);
  }
}

/**
 * @brief  Handles the MAG axes data getting/sending
 * @param  Msg the MAG part of the stream
 * @retval None
 */
static void Magneto_Sensor_Handler(TMsg *Msg)
{
  float ans_float;
  MFX_MagCal_input_t mag_data_in;
  MFX_MagCal_output_t mag_data_out;

  if ((SensorsEnabled & MAGNETIC_SENSOR) == MAGNETIC_SENSOR)
  {
    BSP_SENSOR_MAG_GetAxes(&MagValue);

    if (MagCalStatus == 0U)
    {
      mag_data_in.mag[0] = (float)MagValue.x * FROM_MGAUSS_TO_UT50;
      mag_data_in.mag[1] = (float)MagValue.y * FROM_MGAUSS_TO_UT50;
      mag_data_in.mag[2] = (float)MagValue.z * FROM_MGAUSS_TO_UT50;

      mag_data_in.time_stamp = (int)MagTimeStamp;
      MagTimeStamp += (uint32_t)ALGO_PERIOD;

      MotionFX_manager_MagCal_run(&mag_data_in, &mag_data_out);

      if (mag_data_out.cal_quality == MFX_MAGCALGOOD)
      {
        MagCalStatus = 1;

        ans_float = (mag_data_out.hi_bias[0] * FROM_UT50_TO_MGAUSS);
        MagOffset.x = (int32_t)ans_float;
        ans_float = (mag_data_out.hi_bias[1] * FROM_UT50_TO_MGAUSS);
        MagOffset.y = (int32_t)ans_float;
        ans_float = (mag_data_out.hi_bias[2] * FROM_UT50_TO_MGAUSS);
        MagOffset.z = (int32_t)ans_float;

        /* Disable magnetometer calibration */
        MotionFX_manager_MagCal_stop(ALGO_PERIOD);
      }
    }

    MagValue.x = (int32_t)(MagValue.x - MagOffset.x);
    MagValue.y = (int32_t)(MagValue.y - MagOffset.y);
    MagValue.z = (int32_t)(MagValue.z - MagOffset.z);

    Serialize_s32(&Msg->Data[43], MagValue.x, 4);
    Serialize_s32(&Msg->Data[47], MagValue.y, 4);
    Serialize_s32(&Msg->Data[51], MagValue.z, 4);
  }
}

/**
 * @brief  Handles the PRESS sensor data getting/sending.
 * @param  Msg the PRESS part of the stream
 * @retval None
 */
static void Pressure_Sensor_Handler(TMsg *Msg)
{
  float press_value;

  if ((SensorsEnabled & PRESSURE_SENSOR) == PRESSURE_SENSOR)
  {
    BSP_SENSOR_PRESS_GetValue(&press_value);
    (void)memcpy(&Msg->Data[7], (void *)&press_value, sizeof(float));
  }
}

/**
 * @brief  Handles the TEMP axes data getting/sending
 * @param  Msg the TEMP part of the stream
 * @retval None
 */
static void Temperature_Sensor_Handler(TMsg *Msg)
{
  float temp_value;

  if ((SensorsEnabled & TEMPERATURE_SENSOR) == TEMPERATURE_SENSOR)
  {
    BSP_SENSOR_TEMP_GetValue(&temp_value);
    (void)memcpy(&Msg->Data[11], (void *)&temp_value, sizeof(float));
  }
}

/**
 * @brief  Handles the HUM axes data getting/sending
 * @param  Msg the HUM part of the stream
 * @retval None
 */
static void Humidity_Sensor_Handler(TMsg *Msg)
{
  float hum_value;

  if ((SensorsEnabled & HUMIDITY_SENSOR) == HUMIDITY_SENSOR)
  {
    BSP_SENSOR_HUM_GetValue(&hum_value);
    (void)memcpy(&Msg->Data[15], (void *)&hum_value, sizeof(float));
  }
}

/**
 * @brief  Timer configuration
 * @param  Freq the desired Timer frequency
 * @retval None
 */
static void TIM_Config(uint32_t Freq)
{
  const uint32_t tim_counter_clock = 2000; /* TIM counter clock 2 kHz */
  uint32_t prescaler_value = (uint32_t)((SystemCoreClock / tim_counter_clock) - 1);
  uint32_t period = (tim_counter_clock / Freq) - 1;

  BSP_IP_TIM_Handle.Init.Prescaler         = prescaler_value;
  BSP_IP_TIM_Handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
  BSP_IP_TIM_Handle.Init.Period            = period;
  BSP_IP_TIM_Handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  BSP_IP_TIM_Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&BSP_IP_TIM_Handle) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief  Initialize DWT register for counting clock cycles purpose
 * @param  None
 * @retval None
 */
static void DWT_Init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; /* Disable counter */
}

/**
 * @brief  Start counting clock cycles
 * @param  None
 * @retval None
 */
static void DWT_Start(void)
{
  DWT->CYCCNT = 0; /* Clear count of clock cycles */
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; /* Enable counter */
}

/**
 * @brief  Stop counting clock cycles and calculate elapsed time in [us]
 * @param  None
 * @retval Elapsed time in [us]
 */
static uint32_t DWT_Stop(void)
{
  volatile uint32_t cycles_count = 0U;
  uint32_t system_core_clock_mhz = 0U;

  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; /* Disable counter */
  cycles_count = DWT->CYCCNT; /* Read count of clock cycles */

  /* Calculate elapsed time in [us] */
  system_core_clock_mhz = SystemCoreClock / 1000000U;
  return cycles_count / system_core_clock_mhz;
}

#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
