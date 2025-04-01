#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM patch;

float volume = 1;
float depth = 0;
float rate = 1;
float lfo = 1;

float dial2 = 0;

daisysp::Oscillator LFO;


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	patch.ProcessAllControls();
	volume = patch.GetAdcValue(CV_1);

	dial2 = patch.GetAdcValue(CV_2);
	LFO.SetFreq(dial2  * 10);

	for (size_t i = 0; i < size; i++)
	{
		lfo = LFO.Process();

		OUT_L[i] = IN_L[i] * lfo * volume;
		OUT_R[i] = IN_R[i] * lfo * volume;
	}
}

int main(void)
{
	patch.Init();
	LFO.Init(96000.);
	LFO.SetFreq(1);
	LFO.SetAmp(1);


	patch.SetAudioBlockSize(4); // number of samples handled per callback
	patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
	patch.StartAudio(AudioCallback);
	while(1) {}
}
