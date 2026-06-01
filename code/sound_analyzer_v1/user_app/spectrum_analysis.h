/**
 * @file
 *
 * @brief Signal acquisition and analysis library, in order to get the sound spectrum of the environment
 */

#ifndef SPECTRUM_ANALYSIS_H_
#define SPECTRUM_ANALYSIS_H_

#include <stdbool.h>
#include <stdint.h>
#include "main.h"
#include "arm_math.h"

#define DOUBLE_BUFFER_SIZE 4096
#define HALF_DOUBLE_BUFFER_SIZE DOUBLE_BUFFER_SIZE/2
#define FFT_SIZE 1024
#define HALF_FFT_SIZE FFT_SIZE/2
#define SAMPLING_RATE 47872//~48000 , taken from CubeMX

struct spectrum_data{
	volatile bool is_input_data_ready;
	bool is_mic_buffer_filled;
	bool is_fft_done;
	arm_rfft_fast_instance_f32 hfft;
	void (*process_input)(struct spectrum_data *);
	void (*compute_spectral_data)(struct spectrum_data *);
	void (*mag_to_db)(struct spectrum_data *);
	volatile uint16_t* buf_ptr; //pointer to each half of buffer
	uint16_t raw_mic_data[DOUBLE_BUFFER_SIZE];
	float32_t processed_mic_data[FFT_SIZE];
	float32_t magn_buffer[HALF_FFT_SIZE];
	float32_t dB_data[HALF_FFT_SIZE];
};

typedef struct spectrum_data spectrum_data_t;

void spectrum_init(spectrum_data_t*);

extern spectrum_data_t spectrum;

#endif /* SPECTRUM_ANALYSIS_H_ */
