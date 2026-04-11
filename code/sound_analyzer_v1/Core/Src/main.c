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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include <string.h>
#include <stdbool.h>
#include "arm_math.h"
#include "hanning.h"
//#include "hanning.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ARM_MATH_CM4

#define DOUBLE_BUFFER_SIZE 8192
#define HALF_DOUBLE_BUFFER_SIZE DOUBLE_BUFFER_SIZE/2
#define FFT_SIZE  DOUBLE_BUFFER_SIZE/4 //2048
#define HALF_FFT_SIZE FFT_SIZE/2
#define SAMPLING_RATE 47872//~48000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_spi2_rx;

/* USER CODE BEGIN PV */
uint16_t mic_data[DOUBLE_BUFFER_SIZE];
static volatile uint16_t * buf_ptr = &mic_data[0]; //pointer to each half of buffer

volatile bool data_ready_flag = false;
volatile bool buffer_filled = false;
volatile bool fft_done = false;

arm_rfft_fast_instance_f32 hfft;
uint8_t     ifft_flag                = 0;
const float  frequency_resolution    = (float)SAMPLING_RATE / (float)FFT_SIZE;

static float input_data[FFT_SIZE];
static float fft_power[HALF_FFT_SIZE];

uint32_t last_send = 0;
uint32_t refresh_time = 30; //ms
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2S2_Init(void);
/* USER CODE BEGIN PFP */

static void process_input_data(void);
static void compute_frequencies(void);
static void show_values(void);
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
#if 0
  //Enable cycle counter
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  volatile uint32_t t1;
  volatile uint32_t t2;
  volatile uint32_t diff;
#endif
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S2_Init();
  MX_USB_Device_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(1000);
  HAL_GPIO_WritePin(LED_DBG_GPIO_Port,LED_DBG_Pin,GPIO_PIN_RESET);
  HAL_Delay(1000);
  HAL_GPIO_WritePin(LED_DBG_GPIO_Port,LED_DBG_Pin,GPIO_PIN_SET);
  //HAL_Delay(1000);
  //HAL_GPIO_WritePin(LED_DBG_GPIO_Port,LED_DBG_Pin,GPIO_PIN_RESET);

  //arm_rfft_fast_init_f32(&hfft, FFT_SIZE);
  arm_rfft_fast_init_2048_f32(&hfft); //Use specific size for less build code size

  //Start reading from microphones
  if(HAL_I2S_Receive_DMA(&hi2s2,mic_data,HALF_DOUBLE_BUFFER_SIZE) != HAL_OK){
	  Error_Handler();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(data_ready_flag){
		  data_ready_flag = false;
		  process_input_data();
	  }

	  if(buffer_filled){
		  buffer_filled = false;
		  compute_frequencies();
	  }

	  if(((HAL_GetTick()-last_send)>=refresh_time) && fft_done){
		  last_send = HAL_GetTick();
		  fft_done = false;
		  show_values();
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 18;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV6;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2S2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S2_Init(void)
{

  /* USER CODE BEGIN I2S2_Init 0 */

  /* USER CODE END I2S2_Init 0 */

  /* USER CODE BEGIN I2S2_Init 1 */

  /* USER CODE END I2S2_Init 1 */
  hi2s2.Instance = SPI2;
  hi2s2.Init.Mode = I2S_MODE_MASTER_RX;
  hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s2.Init.DataFormat = I2S_DATAFORMAT_24B;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_48K;
  hi2s2.Init.CPOL = I2S_CPOL_LOW;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S2_Init 2 */

  /* USER CODE END I2S2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_DBG_GPIO_Port, LED_DBG_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC13 PC14 PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PG10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3
                           PA4 PA5 PA6 PA7
                           PA8 PA9 PA10 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10
                           PB14 PB3 PB4 PB5
                           PB6 PB7 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_14|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_DBG_Pin */
  GPIO_InitStruct.Pin = LED_DBG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_DBG_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief Reform incoming data from stereo 24-bit integer to mono float, to prepare them for FFT
 */
static void process_input_data(void){

	static int index = 0;

	for(size_t i=0; i < HALF_DOUBLE_BUFFER_SIZE; i=i+4){

		//Stereo to mono
		int32_t left_channel = (((int32_t)buf_ptr[i]<<16)|buf_ptr[i+1]);
		int32_t right_channel = (((int32_t)buf_ptr[i+2]<<16)|buf_ptr[i+3]);
		float mono = (float) ((left_channel + right_channel)/2.0);

		input_data[index] = mono;
		index++;
	}

	if(index == FFT_SIZE){
		buffer_filled = true;
		index = 0;
	}
}

/**
 * @brief Apply Fast Fourier Transformation to incoming data and compute Power Spectrum of signal
 */
static void compute_frequencies(void){

	//Temporary buffer used as input/output between actions
	float input_data_windowed[FFT_SIZE];
	float fft_out[FFT_SIZE];
	float mag_buffer[HALF_FFT_SIZE];
	float power_spectrum_now[HALF_FFT_SIZE];

	//Weight for current values in weighted moving average
	const float alpha = 0.8;

	//Apply windowing to time data
	arm_mult_f32(input_data,hanning_window,input_data_windowed,FFT_SIZE);

	//Apply (Real) Fast FFT to input array
	arm_rfft_fast_f32(&hfft, input_data_windowed, fft_out, ifft_flag);

	//Convert complex array from output buffer to magnitude
	arm_cmplx_mag_f32(fft_out, mag_buffer, HALF_FFT_SIZE);

	//Normalize with 1/(N*N) to get Power spectrum
	arm_scale_f32(mag_buffer,1.0/(FFT_SIZE*FFT_SIZE),power_spectrum_now,HALF_FFT_SIZE);

	//Average spectrum, with emphasis on newest sample
	for(int i=0;i<HALF_FFT_SIZE;i++){
		fft_power[i] = power_spectrum_now[i]*alpha + fft_power[i]*(1-alpha);
	}

    fft_done = true;
}

/**
 * @brief Display the results of the frequency transformation
 */
static void show_values(void){

	char ch[12000] = {0};
	int len=0;
		len += snprintf(ch + len, sizeof(ch) - len, "[");
		for (int i = 0; i < HALF_FFT_SIZE; i++) {
			len += snprintf(ch + len, sizeof(ch) - len, "%.1f,", fft_power[i]);
		}
		snprintf(ch + len - 1, sizeof(ch) - len, "]\n");

		CDC_Transmit_FS((uint8_t*) ch, strlen(ch));
}


void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s){
	// Choose first half of double buffer
	buf_ptr = &mic_data[0];

	data_ready_flag = true;
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s){
	// Choose second half of double buffer
	buf_ptr = &mic_data[HALF_DOUBLE_BUFFER_SIZE];

	data_ready_flag = true;
}
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
