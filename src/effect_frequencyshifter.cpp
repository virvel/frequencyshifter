/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <Arduino.h>
#include "effect_frequencyshifter.h"
#include "sqrt_integer.h"
#include "utility/dspinst.h"

extern "C" {
extern const int16_t AudioWaveformSine[257];
}

// 140312 - PAH - slightly faster copy
static void copy_to_fft_buffer(void *destination, const void *source)
{
	const uint16_t *src = (const uint16_t *)source;
	uint32_t *dst = (uint32_t *)destination;

	for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		*dst++ = *src++;  // real sample plus a zero for imaginary
	}
}

static void apply_window_to_fft_buffer(void *buffer, const void *window)
{
	int16_t *buf = (int16_t *)buffer;
	const int16_t *win = (int16_t *)window;;

	for (int i=0; i < 256; i++) {
		int32_t val = *buf * *win++;
		//*buf = signed_saturate_rshift(val, 16, 15);
		*buf = val >> 15;
		buf += 2;
	}
}

void AudioFrequencyShifter::update(void)
{
	audio_block_t *block;

	block = receiveWritable(0);
	if (!block) return;
	if (!prevblock) {
		prevblock = block;
		return;
	}
	copy_to_fft_buffer(buffer, prevblock->data);
	copy_to_fft_buffer(buffer+256, block->data);

	//if (window) apply_window_to_fft_buffer(buffer, window);
	arm_cfft_radix4_q15(&fft_inst, buffer);

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

	arm_cfft_radix4_q15(&ifft_inst, buffer);
	//copy_to_analytic_buffer(real, imag, buffer);

	
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
			//cos[j] = multiply_32x32_rshift32(re_val1 + re_val2, magnitude);
			//sin[j] = multiply_32x32_rshift32(im_val1 + im_val2, magnitude);
			prevblock->data[j] = buffer[2*j]*multiply_32x32_rshift32(re_val1 + re_val2, magnitude);
			prevblock->data[j] -= buffer[2*j+1]*multiply_32x32_rshift32(im_val1 + im_val2, magnitude);
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

	/*
	uint32_t *pa, *pb, *end;
	uint32_t a12, a34; //, a56, a78;
	uint32_t b12, b34; //, b56, b78;
	pa = (uint32_t *)(real);
	pb = (uint32_t *)(cos);
	end = pa + AUDIO_BLOCK_SAMPLES/2;
	while (pa < end) {
		a12 = *pa;
		a34 = *(pa+1);
		//a56 = *(pa+2); // 8 samples/loop should work, but crashes.
		//a78 = *(pa+3); // why?!  maybe a compiler bug??
		b12 = *pb++;
		b34 = *pb++;
		//b56 = *pb++;
		//b78 = *pb++;
		a12 = pack_16b_16b(
			signed_saturate_rshift(multiply_16tx16t(a12, b12), 16, 15), 
			signed_saturate_rshift(multiply_16bx16b(a12, b12), 16, 15));
		a34 = pack_16b_16b(
			signed_saturate_rshift(multiply_16tx16t(a34, b34), 16, 15), 
			signed_saturate_rshift(multiply_16bx16b(a34, b34), 16, 15));
		//a56 = pack_16b_16b(
		//	signed_saturate_rshift(multiply_16tx16t(a56, b56), 16, 15), 
		//	signed_saturate_rshift(multiply_16bx16b(a56, b56), 16, 15));
		//a78 = pack_16b_16b(
		//	signed_saturate_rshift(multiply_16tx16t(a78, b78), 16, 15), 
		//	signed_saturate_rshift(multiply_16bx16b(a78, b78), 16, 15));
		*pa++ = a12;
		*pa++ = a34;
		//*pa++ = a56;
		//*pa++ = a78;
	}*/
	
	/*for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
		prevblock->data[i] = cos[i]*real[i] - sin[i]*imag[i];
		//prevblock->data[i] = cos[i];
		//block->data[i] = real[i];
	}*/

	transmit(prevblock);
	release(prevblock);
	prevblock = block;

}


