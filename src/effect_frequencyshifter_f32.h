/* 
* Frequency shifter using FFT based Hilbert transform
 */

#ifndef effect_frequencyshifter_f32_h_
#define effect_frequencyshifter_f32_h_

#include "AudioStream_F32.h"
#include "arm_math.h"

#define NO_WINDOW 0
#define AudioWindowNone 0
#define AudioWindowHanning256 1
#define AudioWindowKaiser256 2
#define AudioWindowBlackmanHarris256 3

class AudioFrequencyShifter_F32 : public AudioStream_F32
{
public:
	AudioFrequencyShifter_F32() : AudioStream_F32(1, inputQueueArray_f32) {
		arm_cfft_radix4_init_f32(&fft_inst, 256, 0, 1);
		arm_cfft_radix4_init_f32(&ifft_inst, 256, 1, 1);
		i_phase_accumulator = 0.;
		q_phase_accumulator = 270.0 * (4294967296.0 / 360.0);
		prevblock = NULL;
		useHanningWindow();
	}
	void frequency(float freq) {
		if (freq < 0.0) freq = 0.0;
		else if (freq > AUDIO_SAMPLE_RATE_EXACT/2) freq = AUDIO_SAMPLE_RATE_EXACT/2;
		phase_increment = freq * (4294967296.0 / AUDIO_SAMPLE_RATE_EXACT);
	}
	void amplitude(float n) {
		if (n < 0) n = 0;
		else if (n > 1.0) n = 1.0;
		magnitude = n * 65536.0;
	}

	virtual void update(void);


private:

	uint32_t i_phase_accumulator;
	uint32_t q_phase_accumulator;
	uint32_t phase_increment;
	int32_t magnitude;

	float buffer[512];
	float real[256];
	float imag[256];
	float cos[128];
	float sin[128];
	float window[256];

	audio_block_f32_t *prevblock;
	audio_block_f32_t *inputQueueArray_f32[1];

	arm_cfft_radix4_instance_f32 fft_inst;
	arm_cfft_radix4_instance_f32 ifft_inst;

    void useHanningWindow(void) {
        for (int i=0; i < 256; i++) {
           // 2*PI/255 = 0.0246399424
           window[i] = 0.5*(1.0 - cosf(0.0246399424*(float)i));
        }
    }
};

#endif
