/* 
* Frequency shifter using Allpass based Hilbert transform
More info:
http://yehar.com/blog/?p=368
 */

#include <Arduino.h>
#include "effect_frequencyshifter_f32.h"
#include "sqrt_integer.h"
#include "utility/dspinst.h"

extern "C" {
extern const int16_t AudioWaveformSine[257];
}


void AudioFrequencyShifter_F32::update()
{
	i_block = receiveWritable_f32(0);
	if (!i_block) return;

	for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
		q[i] = i_block->data[i];

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
			cos[j] = (float) (multiply_32x32_rshift32(re_val1 + re_val2, magnitude)) / 32767.0;
			sin[j] = (float) (multiply_32x32_rshift32(im_val1 + im_val2, magnitude)) / 32767.0;
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

	
 	arm_biquad_cascade_df1_f32(&i_ap, i_block->data, i_block->data, 128);
 	arm_biquad_cascade_df1_f32(&q_ap, q, q, 128);

	// Ring modulation

	arm_mult_f32(cos, i_block->data, i_block->data, 128);
	arm_scale_f32(i_block->data, 0.5, i_block->data, 128);

	arm_mult_f32(sin, q, q, 128);
	arm_scale_f32(q, 0.5, q, 128);

	arm_sub_f32(i_block->data, q, i_block->data, 128);
	
	// Transmit and release resources

	transmit(i_block);
	release(i_block);
	/*
	release(prevblock);
	prevblock = block;
	*/

}


