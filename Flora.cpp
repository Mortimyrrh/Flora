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
float lfo_value = 1;
float lfo_freq = 1;

float delay_time = 0;

float delayed_L = 0;
float delayed_R = 0;

float feedback_L = 0;
float feedback_R = 0;

float feedback = 0;

float wet_L = 0;
float wet_R = 0;

float dry_wet_mix = 0;
float dry_mix = 0;
float wet_mix = 0;

bool pingpong = false;

daisysp::Oscillator lfo;
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

	volume = 0.2;
	fonepole(delay_time, dial1 * Fs * 0.5f, .0002f);
	feedback = dial2 * 1;
	// lfo_freq = dial3; TODO: LP cutoff
	// lfo.SetFreq(lfo_freq  * 10);
	filter_cutoff = 20 + ((1 + ((log(dial3) * 0.4))) * 22000); // This is a bit janky no log scaling
	dry_wet_mix = dial4;
	filter_q = 0.5;
	
	pingpong = !toggle.Pressed();

	low_pass_filter_L.SetFreq(filter_cutoff);
	low_pass_filter_R.SetFreq(filter_cutoff);

	low_pass_filter_L.SetRes(filter_q);
	low_pass_filter_R.SetRes(filter_q);

	for (size_t i = 0; i < size; i++)
	{
		lfo_value = lfo.Process();
		 // delay.SetDelay(delay_time);
		delayed_L = delay_L.ReadHermite(delay_time);
		delayed_R = delay_R.ReadHermite(delay_time);

		//
		
		feedback_L = (pingpong ? IN_R[i] : IN_L[i]) * feedback;
		feedback_R = (pingpong ? IN_L[i] : IN_R[i]) * feedback;

		low_pass_filter_L.Process(feedback_L);
		low_pass_filter_R.Process(feedback_R);

		delay_L.Write(IN_L[i] + low_pass_filter_L.Low());
		delay_R.Write(IN_R[i] + low_pass_filter_R.Low());

		wet_L = delayed_L;
		wet_R = delayed_R;

		// https://dsp.stackexchange.com/questions/14754/equal-power-crossfade
		// Need to read this: https://dafx16.vutbr.cz/dafxpapers/16-DAFx-16_paper_07-PN.pdf
		dry_mix = sqrt(0.5f * (1.f + (1.f - dry_wet_mix)));
		wet_mix = sqrt(0.5f * (1.f - (1.f - dry_wet_mix)));

		OUT_L[i] = ((wet_L * wet_mix) + (IN_L[i] * dry_mix)) * volume;
		OUT_R[i] = ((wet_R * wet_mix) + (IN_R[i] * dry_mix)) * volume;

	}
}

int main(void)
{
	patch.Init();
	lfo.Init(Fs);
	lfo.SetFreq(1);
	lfo.SetAmp(1);
	
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
