/* 
* Frequency shifter using FFT based Hilbert transform
 */

#include <Arduino.h>
#include "effect_frequencyshifter_f32.h"
#include "sqrt_integer.h"
#include "utility/dspinst.h"

extern "C" {
extern const int16_t AudioWaveformSine[257];
}

static void copy_to_fft_buffer(void *destination, const void *source)  {
    const float *src = (const float *)source;
    float *dst = (float *)destination;
    for (int i=0; i < AUDIO_BLOCK_SAMPLES; ++i) {
       *dst++ = *src++;
       *src++;
       }
    }

static void apply_window_to_fft_buffer(void *fft_buffer, const void *window) {
    float *buf = (float *)fft_buffer;      // 0th entry is real (do window) 1th is imag
    const float *win = (float *)window;
    for (int i=0; i < 256; i++)  {
       buf[2*i] *= *win;      // real
       buf[2*i + 1] *= *win++;  // imag
       }
    }

 static void copy_to_analytic_buffer(void *real, void *imag, const void *source) {
 	float *src = (float *) source;
 	float *re = (float*) real;
 	float *im = (float*) imag;
 	for (int i = 0; i< AUDIO_BLOCK_SAMPLES; ++i) {
 		*re++ = *src++;
 		*im++ = *src++;
 	}
 }

void AudioFrequencyShifter_F32::update()
{
	audio_block_f32_t *block;

	block = receiveWritable_f32(0);
	if (!block) return;
	if (!prevblock) {
		prevblock = block;
		return;
	}
	copy_to_fft_buffer(buffer, prevblock->data);
	copy_to_fft_buffer(buffer+256, block->data);

	//if (window) apply_window_to_fft_buffer(buffer, window);
	arm_cfft_radix4_f32(&fft_inst, buffer);

	uint32_t L, i;

	L = (fft_inst.fftLen);

	for (i = 0;i < (L*2); i+=2) {
		if(i == L) {}
		else if (i <= L-2) {
			buffer[i] *= 2;
			buffer[i+1] *= 2;
		}
		else {
			buffer[i] = 0;
			buffer[i+1] = 0;
		}
	}

	arm_cfft_radix4_f32(&ifft_inst, buffer);

	copy_to_analytic_buffer(real, imag, buffer);

	// Quadrature oscillator for ring modulation

	uint32_t j, i_ph, q_ph, inc, index, scale;
	int32_t re_val1, re_val2, im_val1, im_val2;

	if (magnitude) {
		i_ph = i_phase_accumulator;
		q_ph = q_phase_accumulator;
		inc = phase_increment;
		for (j=0; j < AUDIO_BLOCK_SAMPLES; j++) {

			index = i_ph >> 24;
			re_val1 = AudioWaveformSine[index];
			re_val2 = AudioWaveformSine[index+1];
			scale = (i_ph >> 8) & 0xFFFF;
			re_val2 *= scale;
			re_val1 *= 0x10000 - scale;

			index = q_ph >> 24;
			im_val1 = AudioWaveformSine[index];
			im_val2 = AudioWaveformSine[index+1];
			scale = (q_ph >> 8) & 0xFFFF;
			im_val2 *= scale;
			im_val1 *= 0x10000 - scale;

#if defined(__ARM_ARCH_7EM__)
			cos[j] = multiply_32x32_rshift32(re_val1 + re_val2, magnitude) / 32768.0;
			sin[j] = multiply_32x32_rshift32(im_val1 + im_val2, magnitude) / 32768.0;
			//prevblock->data[j] = buffer[2*j]*multiply_32x32_rshift32(re_val1 + re_val2, magnitude) / 32768.0;
			//prevblock->data[j] -= buffer[2*j+1]*multiply_32x32_rshift32(im_val1 + im_val2, magnitude) / 32768.0;
#elif defined(KINETISL)
			cos[j] *= (((re_val1 + re_val2) >> 16) * magnitude) >> 16;
			sin[j] *= (((im_val1 + im_val2) >> 16) * magnitude) >> 16;
#endif
			i_ph += inc;
			q_ph += inc;
		}
		i_phase_accumulator = i_ph;
		q_phase_accumulator = q_ph;
	}
	else {
		i_phase_accumulator += phase_increment * AUDIO_BLOCK_SAMPLES;
		q_phase_accumulator += phase_increment * AUDIO_BLOCK_SAMPLES;
	}

	// Ring modulation

	arm_mult_f32(cos, real, real,128);
	arm_mult_f32(sin, imag, imag, 128);
	arm_sub_f32(real, imag, prevblock->data, 128);

	// Transmit and release resources

	transmit(prevblock);
	release(prevblock);
	prevblock = block;

}


