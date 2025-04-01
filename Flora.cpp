#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM patch;

float volume = 1;
float depth = 0;
float rate = 1;

daisysp::Chorus chorusL;
daisysp::Chorus chorusR;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	patch.ProcessAllControls();
	volume = patch.GetAdcValue(CV_1); // 0 -> 1

	depth = patch.GetAdcValue(CV_2);
	chorusL.SetLfoDepth(depth);
	chorusR.SetLfoDepth(depth);

	rate = patch.GetAdcValue(CV_3);
	chorusL.SetLfoFreq(rate * 3);
	chorusR.SetLfoFreq(rate * 2.5);

	for (size_t i = 0; i < size; i++)
	{
		chorusL.Process(IN_L[i]);
		chorusR.Process(IN_R[i]);

		OUT_L[i] = chorusL.GetLeft() * volume;
		OUT_R[i] = chorusR.GetRight() * volume;
	}
}

int main(void)
{
	patch.Init();
	chorusL.Init(96000.);
	chorusR.Init(96000.);


	patch.SetAudioBlockSize(4); // number of samples handled per callback
	patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
	patch.StartAudio(AudioCallback);
	while(1) {}
}
