#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM patch;

float volume = 1;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	patch.ProcessAllControls();
	volume = patch.GetAdcValue(CV_1); // 0 -> 1


	for (size_t i = 0; i < size; i++)
	{
		OUT_L[i] = IN_L[i] * volume;
		OUT_R[i] = IN_R[i] * volume;
	}
}

int main(void)
{
	patch.Init();
	patch.SetAudioBlockSize(4); // number of samples handled per callback
	patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
	patch.StartAudio(AudioCallback);
	while(1) {}
}
