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
#include "weighting.h"
#include "window.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ARM_MATH_CM4

#define DOUBLE_BUFFER_SIZE 4096
#define HALF_DOUBLE_BUFFER_SIZE DOUBLE_BUFFER_SIZE/2
#define FFT_SIZE 1024
#define HALF_FFT_SIZE FFT_SIZE/2
#define SAMPLING_RATE 47872//~48000 , taken from CubeMX

#define USE_FLATTOP_WINDOW
//#define USE_HANNING_WINDOW
//#define USE_RECTANGULAR_WINDOW

#define USE_A_WEIGHTING
//#define USE_C_WEIGHTING

#if ((!defined(USE_FLATTOP_WINDOW))&&(!defined(USE_HANNING_WINDOW))&&(!defined(USE_RECTANGULAR_WINDOW)) || (defined(USE_HANNING_WINDOW)&&defined(USE_RECTANGULAR_WINDOW))|| (defined(USE_FLATTOP_WINDOW)&&defined(USE_RECTANGULAR_WINDOW)) || (defined(USE_FLATTOP_WINDOW)&&defined(USE_HANNING_WINDOW)))
#error "Please choose at least one between GSM,NBIOT and CATM definition"
#endif

#if defined(USE_A_WEIGHTING) && defined(USE_C_WEIGHTING)
#error "Please choose only 1 between A and C weighting"
#endif

#if (DOUBLE_BUFFER_SIZE % FFT_SIZE) != 0
#error "Input buffer size shall be integer multiple of FFT size"
#endif
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_spi2_rx;

/* USER CODE BEGIN PV */
static uint16_t mic_data[DOUBLE_BUFFER_SIZE];
static volatile uint16_t *buf_ptr = &mic_data[0]; //pointer to each half of buffer

static volatile bool data_ready_flag = false;
static volatile bool buffer_filled = false;
static volatile bool fft_done = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2S2_Init(void);
/* USER CODE BEGIN PFP */

static void process_input_data(float32_t*);
static void compute_frequencies(arm_rfft_fast_instance_f32*,float32_t*,float32_t*);
static void scale_frequencies(float32_t* , float32_t* );
static void show_values(float32_t*, size_t );
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

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
  //Enable cycle counter for measuring time duration
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  volatile uint32_t t1;
  volatile uint32_t t2;
  volatile uint32_t diff;
  const uint32_t sys_clock_freq = HAL_RCC_GetSysClockFreq();
#endif
	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_I2S2_Init();
	MX_USB_Device_Init();
	/* USER CODE BEGIN 2 */
	arm_rfft_fast_instance_f32 hfft;
	//const float32_t frequency_resolution = (float32_t) SAMPLING_RATE / (float32_t) FFT_SIZE;

	float32_t input_td[FFT_SIZE] = { 0 };
	float32_t fft_mag[HALF_FFT_SIZE] = { 0 };
	float32_t output_db[HALF_FFT_SIZE] = { 0 };

	uint32_t last_send = 0;
	uint32_t refresh_time = 10; //ms

	HAL_GPIO_WritePin(LED_DBG_GPIO_Port, LED_DBG_Pin, GPIO_PIN_SET);
	HAL_Delay(500);
	HAL_GPIO_WritePin(LED_DBG_GPIO_Port, LED_DBG_Pin, GPIO_PIN_RESET);
	HAL_Delay(500);

	//arm_rfft_fast_init_f32(&hfft, FFT_SIZE);
#if (FFT_SIZE != 1024)
#error "Please use respective arm_rfft_fast_init_XXX_f32() function"
#else
	arm_rfft_fast_init_1024_f32(&hfft); //Use specific size for less build code size
#endif

	//Start reading from microphones
	if (HAL_I2S_Receive_DMA(&hi2s2, mic_data, HALF_DOUBLE_BUFFER_SIZE)
			!= HAL_OK) {
		Error_Handler();
	}

	//Successful initilization
	HAL_GPIO_WritePin(LED_DBG_GPIO_Port, LED_DBG_Pin, GPIO_PIN_SET);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		if (data_ready_flag) {
			data_ready_flag = false;
			process_input_data(input_td);
		}

		if (buffer_filled) {
			buffer_filled = false;
			compute_frequencies(&hfft, input_td,fft_mag);
		}

		if (((HAL_GetTick() - last_send) >= refresh_time) && fft_done && tx_complete) {
			last_send = HAL_GetTick();
			fft_done = false;
			tx_complete = false;
			scale_frequencies(fft_mag, output_db);
			show_values(output_db, HALF_FFT_SIZE);
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
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
	RCC_OscInitStruct.PLL.PLLN = 24;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV6;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2S2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2S2_Init(void) {

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
	if (HAL_I2S_Init(&hi2s2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2S2_Init 2 */

	/* USER CODE END I2S2_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {

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
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
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
	GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
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
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
			| GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8
			| GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : PB0 PB1 PB2 PB10
	 PB14 PB3 PB4 PB5
	 PB6 PB7 PB8 PB9 */
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_10
			| GPIO_PIN_14 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6
			| GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
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
 * param[out] input_td Array to store time-domain data from microphone
 */
static void process_input_data(float32_t* mic_data) {

	static int index = 0;
	uint16_t input_data[HALF_DOUBLE_BUFFER_SIZE];
	static const float32_t full_range = 0x7fffff; //2^23 - 1

	//Copy data to avoid corruption
	memcpy(input_data, (uint16_t*) buf_ptr, sizeof(input_data));

	for (size_t i = 0; i < HALF_DOUBLE_BUFFER_SIZE; i = i + 4) {
		// Get data from I2S for each channel
		int32_t left_channel = (((int32_t) input_data[i] << 16)
				| input_data[i + 1]);
		int32_t right_channel = (((int32_t) input_data[i + 2] << 16)
				| input_data[i + 3]);

		//Keep only 24bits and take care of sign
		left_channel &= 0xFFFFFF;
		right_channel &= 0xFFFFFF;
		if (left_channel & 0x800000) {
			left_channel |= ~0xFFFFFF;
		}
		if (right_channel & 0x800000) {
			right_channel |= ~0xFFFFFF;
		}

		//Stereo to mono ( (L+R)/2 ) & normalize to Full Scale [-1.0,1.0]
		float32_t mono = ((float32_t) (left_channel + right_channel) / 2.0)
				/ full_range;

		mic_data[index] = mono;
		index++;
	}

	if (index == FFT_SIZE) {
		buffer_filled = true;
		index = 0; // set for next iteration
	}

}

/**
 * @brief Apply Fast Fourier Transformation to incoming data and compute spectral content of signal
 * @param[in] hfft FFT handler
 * @param[in] input_td Time-domain data from microphone
 * @param[out] out_freq dBFS signal values in frequency
 */
static void compute_frequencies(arm_rfft_fast_instance_f32* hfft,float32_t* input_td,float32_t* output_freq) {

	//Intermediate buffers used as input/output between actions
	float32_t input_td_windowed[FFT_SIZE] = { 0 };
	float32_t fft_out[FFT_SIZE] = { 0 };
	float32_t mag_now[HALF_FFT_SIZE] = { 0 };
	float32_t temp[HALF_FFT_SIZE] = { 0 };
	static const uint8_t ifft_flag = 0;

	//Weight for current values in weighted moving average
	static const float32_t alpha = 0.5;

	//Window functions
#if defined(USE_HANNING_WINDOW)
	static float32_t const* window = hanning;
	static float32_t sum_of_window = sum_hanning;
#elif defined(USE_FLATTOP_WINDOW)
	static float32_t const* window = flattop;
	static const float32_t sum_of_window = sum_flattop;
#else
	static float32_t const* window = rectangular;
	static float32_t sum_of_window = sum_rectangular;
#endif

	//Remove DC Offset
	float32_t mean;
	arm_mean_f32(input_td, FFT_SIZE, &mean);
	arm_offset_f32(input_td, -mean, temp, FFT_SIZE);

	//Apply windowing to time data
	arm_mult_f32(temp, window, input_td_windowed, FFT_SIZE);

	//Apply (Real) Fast FFT to input array
	arm_rfft_fast_f32(hfft, input_td_windowed, fft_out, ifft_flag);

	//Compute magnitude from complex FFT array
	arm_cmplx_mag_f32(fft_out, temp, HALF_FFT_SIZE);

	//Normalize magnitude, for one-sided FFT and windowing
	arm_scale_f32(temp, 2.0/sum_of_window , mag_now, HALF_FFT_SIZE);

	//Average spectrum values
	for (int i = 0; i < HALF_FFT_SIZE; i++) {
		output_freq[i] = mag_now[i] * alpha + output_freq[i] * (1 - alpha);
	}

	fft_done = true;
}

/**
 *@brief Scale magnitude to a real-world Decibel value
 *@param[in] input_freq Result of FFT , in magnitude
 *@param[out] output_db data after transpose to decibel scale
 */
static void scale_frequencies(float32_t* input_freq, float32_t* output_db){

	float32_t dbfs[HALF_FFT_SIZE] = {0};
	float32_t temp[HALF_FFT_SIZE] = { 0 };

	//Compute dBFS from magnitude - break the computation to steps with pre-computed values for code optimization
	//dBFS = 20*log10(mag) = 20*(ln(mag*)/ln(10))
	arm_vlog_f32(input_freq, temp, HALF_FFT_SIZE); // = ln(mag)
	arm_scale_f32(temp, 8.6858896380, dbfs, HALF_FFT_SIZE); // = (20/ln(10))*ln(mag) = 8.685889638*ln(mag)

	//Offset to dBSPL, based on calibration from mic specification
#if defined(USE_A_WEIGHTING)
	//Apply A-Weighting to dBSPL
	arm_offset_f32(dbfs, 120, temp, HALF_FFT_SIZE);
	arm_add_f32(temp,a_weighting_db,output_db,HALF_FFT_SIZE);
#elif defined(USE_C_WEIGHTING)
	//Apply C-Weighting to dBSPL
	arm_offset_f32(dbfs, 120, temp, HALF_FFT_SIZE);
	arm_add_f32(temp,c_weighting_db,output_db,HALF_FFT_SIZE);
#else
	arm_offset_f32(dbfs, 120, output_db, HALF_FFT_SIZE);
#endif
}


/**
 * @brief Display the results of the frequency computation
 * @param[in] data_out Output data to send to host
 */
static void show_values(float32_t* data_out, size_t data_out_sz) {

	char str_to_send[8192] = { 0 }; // arbitary large size
	int len = 0;

	// Group data to format '[I0.D0, I1.D1, ... IXX,DXX]'
	len += snprintf(str_to_send + len, sizeof(str_to_send) - len, "[");
	for (int i = 0; i < data_out_sz; i++) {
		len += snprintf(str_to_send + len, sizeof(str_to_send) - len, "%.1f,",
				data_out[i]);
	}
	snprintf(str_to_send + len - 1, sizeof(str_to_send) - len, "]\n");

	//Send them to host PC via USB
	CDC_Transmit_FS((uint8_t*) str_to_send, strlen(str_to_send));
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
	// Choose first half of double buffer
	buf_ptr = &mic_data[0];

	data_ready_flag = true;
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s) {
	// Choose second half of double buffer
	buf_ptr = &mic_data[HALF_DOUBLE_BUFFER_SIZE];

	data_ready_flag = true;
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
		HAL_GPIO_WritePin(LED_DBG_GPIO_Port, LED_DBG_Pin, GPIO_PIN_SET);
		HAL_Delay(100);
		HAL_GPIO_WritePin(LED_DBG_GPIO_Port, LED_DBG_Pin, GPIO_PIN_RESET);
		HAL_Delay(100);
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
