/**
 * @file
 *
 * @brief Signal acquisition and analysis library, in order to get the sound spectrum of the environment
 */
#include "spectrum_analysis.h"
#include "weighting.h"
#include "window.h"

/** -- Choose some of the following values to define device behaviour -- **/

#define USE_FLATTOP_WINDOW
//#define USE_HANNING_WINDOW
//#define USE_RECTANGULAR_WINDOW

#define USE_A_WEIGHTING
//#define USE_C_WEIGHTING

/** -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- **/

#if ((!defined(USE_FLATTOP_WINDOW))&&(!defined(USE_HANNING_WINDOW))&&(!defined(USE_RECTANGULAR_WINDOW)) || (defined(USE_HANNING_WINDOW)&&defined(USE_RECTANGULAR_WINDOW))|| (defined(USE_FLATTOP_WINDOW)&&defined(USE_RECTANGULAR_WINDOW)) || (defined(USE_FLATTOP_WINDOW)&&defined(USE_HANNING_WINDOW)))
#error "Please choose at least one between GSM,NBIOT and CATM definition"
#endif

#if defined(USE_A_WEIGHTING) && defined(USE_C_WEIGHTING)
#error "Please choose only 1 between A and C weighting"
#endif

#if (DOUBLE_BUFFER_SIZE % FFT_SIZE) != 0
#error "Input buffer size shall be integer multiple of FFT size"
#endif

static void process_input_data(spectrum_data_t* sd);
static void compute_frequencies(spectrum_data_t* sd);
static void scale_frequencies(spectrum_data_t* sd);

spectrum_data_t spectrum = {
		.buf_ptr = NULL,
		.is_input_data_ready = false,
		.is_mic_buffer_filled = false,
		.is_fft_done = false,
		.process_input = NULL,
		.compute_spectral_data = NULL,
		.mag_to_db = NULL,
		.processed_mic_data = {0},
		.magn_buffer = {0},
		.dB_data = {0}
};


/**
 * @brief Initialize the variables & buffers used for spectrum analysis
 * @param sd Struct containing the required data
 */
void spectrum_init(spectrum_data_t* sd){

	//arm_rfft_fast_init_f32(sd->hfft, FFT_SIZE);
#if (FFT_SIZE != 1024)
#error "Please use respective arm_rfft_fast_init_XXX_f32() function"
#else
	arm_rfft_fast_init_1024_f32(&sd->hfft); //Use specific size for less build code size
#endif

	sd->buf_ptr = &(sd->raw_mic_data)[0];
	sd->process_input = process_input_data;
	sd->compute_spectral_data = compute_frequencies;
	sd->mag_to_db = scale_frequencies;
}



/**
 * @brief Reform incoming data from stereo 24-bit integer to mono float, to prepare them for FFT
 * param[out] input_td Array to store time-domain data from microphone
 */
static void process_input_data(spectrum_data_t* sd) {

	static size_t index = 0;
	uint16_t input_data[HALF_DOUBLE_BUFFER_SIZE];
	static const float32_t full_range = 0x7fffff; //2^23 - 1

	//Copy data to avoid corruption
	memcpy(input_data, (uint16_t*) sd->buf_ptr, sizeof(input_data));

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

		sd->processed_mic_data[index] = mono;
		index++;
	}

	if (index == FFT_SIZE) {
		sd->is_mic_buffer_filled = true;
		index = 0; // set for next iteration
	}

}

/**
 * @brief Apply Fast Fourier Transformation to incoming data and compute spectral content of signal
 * @param[in] hfft FFT handler
 * @param[in] input_td Time-domain data from microphone
 * @param[out] out_freq dBFS signal values in frequency
 */
static void compute_frequencies(spectrum_data_t* sd) {

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
	arm_mean_f32(sd->processed_mic_data, FFT_SIZE, &mean);
	arm_offset_f32(sd->processed_mic_data, -mean, temp, FFT_SIZE);

	//Apply windowing to time data
	arm_mult_f32(temp, window, input_td_windowed, FFT_SIZE);

	//Apply (Real) Fast FFT to input array
	arm_rfft_fast_f32(&sd->hfft, input_td_windowed, fft_out, ifft_flag);

	//Compute magnitude from complex FFT array
	arm_cmplx_mag_f32(fft_out, temp, HALF_FFT_SIZE);

	//Normalize magnitude, for one-sided FFT and windowing
	arm_scale_f32(temp, 2.0/sum_of_window , mag_now, HALF_FFT_SIZE);

	//Average spectrum values
	for (int i = 0; i < HALF_FFT_SIZE; i++) {
		sd->magn_buffer[i] = mag_now[i] * alpha + sd->magn_buffer[i] * (1 - alpha);
	}

	sd->is_fft_done = true;
}

/**
 *@brief Scale magnitude to a real-world Decibel value
 *@param[in] input_freq Result of FFT , in magnitude
 *@param[out] output_db data after transpose to decibel scale
 */
static void scale_frequencies(spectrum_data_t* sd){

	float32_t dbfs[HALF_FFT_SIZE] = {0};
	float32_t temp[HALF_FFT_SIZE] = { 0 };

	//Compute dBFS from magnitude - break the computation to steps with pre-computed values for code optimization
	//dBFS = 20*log10(mag) = 20*(ln(mag*)/ln(10))
	arm_vlog_f32(sd->magn_buffer, temp, HALF_FFT_SIZE); // = ln(mag)
	arm_scale_f32(temp, 8.6858896380, dbfs, HALF_FFT_SIZE); // = (20/ln(10))*ln(mag) = 8.685889638*ln(mag)

	//Offset to dBSPL, based on calibration from mic specification
#if defined(USE_A_WEIGHTING)
	//Apply A-Weighting to dBSPL
	arm_offset_f32(dbfs, 120, temp, HALF_FFT_SIZE);
	arm_add_f32(temp,a_weighting_db,sd->dB_data,HALF_FFT_SIZE);
#elif defined(USE_C_WEIGHTING)
	//Apply C-Weighting to dBSPL
	arm_offset_f32(dbfs, 120, temp, HALF_FFT_SIZE);
	arm_add_f32(temp,c_weighting_db,sd->dB_data,HALF_FFT_SIZE);
#else
	arm_offset_f32(dbfs, 120, sd->dB_data, HALF_FFT_SIZE);
#endif
}
