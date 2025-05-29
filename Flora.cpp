#include "daisy_patch_sm.h"
#include "daisysp.h"

#include <cmath>

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM patch;
Switch button, toggle;


float dial1 = 0;
float dial2 = 0;
float dial3 = 0;
float dial4 = 0;

float volume = 1;
float depth = 0;
float rate = 1;

float lfo1_value = 1;
float lfo1_freq = 1;

float lfo2_value = 1;
float lfo2_freq = 1;

float delay_time = 0;

float delayed_L1 = 0;
float delayed_R1 = 0;

float delayed_L2 = 0;
float delayed_R2 = 0;

float delayed_L3 = 0;
float delayed_R3 = 0;

float feedback_L1 = 0;
float feedback_R1 = 0;

float feedback = 0;

float filter_scale = (22000 - 20) / (1 - 0);
float filtered_L = 0;
float filtered_R = 0;

float limiter_L = 0;
float limiter_R = 0;

float wet_L = 0;
float wet_R = 0;

float dry_wet_mix = 0;
float dry_mix = 0;
float wet_mix = 0;

float lfo_delay_time_1 = 0;
float lfo_delay_time_2 = 0;
float lfo_delay_time_3 = 0;

float vibrato_speed = 0;
float vibrato_depth = 0;

bool pingpong = false;

const float HALF_PI = 1.57079632679;

daisysp::Oscillator lfo1;
daisysp::Oscillator lfo2;
daisysp::Svf low_pass_filter_L;
daisysp::Svf low_pass_filter_R;

float filter_q = 0.5;
float filter_cutoff = 4000.f;


const float Fs = 96000.f; // Sample rate
#define MAX_DELAY static_cast<size_t>(96000 * 0.5f)

daisysp::DelayLine <float, MAX_DELAY> delay_L;
daisysp::DelayLine <float, MAX_DELAY> delay_R;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	patch.ProcessAllControls();
	button.Debounce();
	toggle.Debounce();

	
	dial1 = patch.GetAdcValue(CV_1);
	dial2 = patch.GetAdcValue(CV_2);
	dial3 = patch.GetAdcValue(CV_3);
	dial4 = patch.GetAdcValue(CV_4);

	volume = 0.3;
	// fonepole(delay_time, 0.5 * Fs * 0.5f, .0002f); //0.5 is a hard coded delay should be less and calculated
	// delay_time = (Fs / 1000) * (dial1 * 30); // 0-3ms
	feedback = dial2 * 1.1;

	// lfo_freq = dial3;
	// lfo.SetFreq(lfo_freq  * 10);
	
	vibrato_speed = dial3;
	lfo1.SetFreq(vibrato_speed * 4);
	float phasor_depth = dial4 * 1; //1ms wiggle
	lfo1.SetAmp(phasor_depth);
	// lfo2.SetFreq(vibrato_speed * 9);

	// vibrato_depth = dial1;
	// lfo1.SetAmp(vibrato_depth);
	// lfo2.SetAmp(vibrato_depth);
	
	// // filter_cutoff = dial3 * filter_scale;
	dry_wet_mix = 1; // dial4
	// filter_q = 0;
	

	// filter_cutoff = 0.4 * filter_scale;
	// filter_q = 0;
	
	pingpong = toggle.Pressed();

	// low_pass_filter_L.SetFreq(filter_cutoff);
	// low_pass_filter_R.SetFreq(filter_cutoff);

	// low_pass_filter_L.SetRes(filter_q);
	// low_pass_filter_R.SetRes(filter_q);

	for (size_t i = 0; i < size; i++)
	{
		lfo1_value = lfo1.Process();
		delay_time = (Fs / 1000) * ((dial1 * 3) + phasor_depth + lfo1_value); // 0-3ms with a 1ms lfo 

		// lfo2_value = lfo2.Process();

		// fonepole(lfo_delay_time_1, (delay_time) * Fs * 0.5f, .00002f);
		// fonepole(lfo_delay_time_2, (lfo2_value) * Fs * 0.5f, .00002f);

		delayed_L1 = delay_L.ReadHermite(delay_time);
		delayed_R1 = delay_R.ReadHermite(delay_time);

		// delayed_L2 = delay_L.ReadHermite(delay_time + lfo_delay_time_1);
		// delayed_R2 = delay_R.ReadHermite(delay_time + lfo_delay_time_1);
		
		feedback_L1 = (pingpong ? delayed_L1 : delayed_R1) * feedback; // this will click should use a cross fade
		feedback_R1 = (pingpong ? delayed_R1 : delayed_L1) * feedback;
		
		// low_pass_filter_L.Process(feedback_L);
		// low_pass_filter_R.Process(feedback_R);

		// filtered_R = low_pass_filter_L.Low();
		// filtered_L = low_pass_filter_R.Low();

		// tanh saturation Oo
		limiter_L = tanh(feedback_L1);
		limiter_R = tanh(feedback_R1);

		delay_L.Write(IN_L[i] + limiter_L);
		delay_R.Write(IN_R[i] + limiter_R);

		wet_L = delayed_L1;
		wet_R = delayed_R1;

		// https://dsp.stackexchange.com/questions/14754/equal-power-crossfade
		// Need to read this: https://dafx16.vutbr.cz/dafxpapers/16-DAFx-16_paper_07-PN.pdf
		// https://dsp.stackexchange.com/questions/37477/understanding-equal-power-crossfades
		// https://www.desmos.com/calculator/xxvud7li3d
		// https://www.desmos.com/calculator/q9bd0e0drf

		wet_mix = cos((1 - dry_wet_mix) * HALF_PI);
		dry_mix = cos(dry_wet_mix * HALF_PI);

		OUT_L[i] = ((wet_L * wet_mix) + (IN_L[i] * dry_mix)) * volume;
		OUT_R[i] = ((wet_R * wet_mix) + (IN_R[i] * dry_mix)) * volume;

	}
}

int main(void)
{
	patch.Init();
	lfo1.Init(Fs);
	lfo1.SetFreq(1);
	lfo1.SetAmp(1);

	lfo2.Init(Fs);
	lfo2.SetFreq(1);
	lfo2.SetAmp(1);
	
	delay_L.Init();
	delay_R.Init();

	low_pass_filter_L.Init(Fs);
	low_pass_filter_R.Init(Fs);

	button.Init(DaisyPatchSM::B7, patch.AudioCallbackRate());
	toggle.Init(DaisyPatchSM::B8, patch.AudioCallbackRate());

	patch.SetAudioBlockSize(4); // number of samples handled per callback
	patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
	patch.StartAudio(AudioCallback);
	while(1) {}
}
