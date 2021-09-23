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
		arm_biquad_cascade_df1_init_f32(&i_ap, n_states, coeffs_i, i_state);
		arm_biquad_cascade_df1_init_f32(&q_ap, n_states, coeffs_q, q_state);
se_accumulator = 0.;
		q_phase_accumulator = 270.0 * (4294967296.0 / 360.0);
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

	audio_block_f32_t *i_block;
	audio_block_f32_t *inputQueueArray_f32[1];

	arm_biquad_casd_df1_inst_f32 i_ap;
	arm_biquad_casd_df1_inst_f32 q_ap;

	static const int n_states = 4;

	float i_state[n_states * 4];
	float q_state[n_states * 4];

	const float c_coeffs_i[4] = {0.47944111608296202665,0.87624358989504858020,0.97660296916871658368,0.99749940412203375040};
	const float c_coeffs_q[4] = {0.16177741706363166219,0.73306690130335572242,0.94536301966806279840,0.99060051416704042460};

	float coeffs_i[n_states * 5] = {c_coeffs_i[0], 0.f, -1.f, 0.f, c_coeffs_i[0],
											  c_coeffs_i[1], 0.f, -1.f, 0.f, c_coeffs_i[1],
											  c_coeffs_i[2], 0.f, -1.f, 0.f, c_coeffs_i[2],
											  c_coeffs_i[3], 0.f, -1.f, 0.f, c_coeffs_i[3]};

	float coeffs_q[n_states * 5] = {c_coeffs_q[0], 0.f, -1.f, 0.f, c_coeffs_q[0],
								 			  c_coeffs_q[1], 0.f, -1.f, 0.f, c_coeffs_q[1],
								 			  c_coeffs_q[2], 0.f, -1.f, 0.f, c_coeffs_q[2],
											  c_coeffs_q[3], 0.f, -1.f, 0.f, c_coeffs_q[3]};
								
	float cos[128];
	float sin[128];
	float q[128];

};

#endif
