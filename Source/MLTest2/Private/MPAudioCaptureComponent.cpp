// RIT CSCI-715 Semester 2221 Project


#include "MPAudioCaptureComponent.h"

int32 UMPAudioCaptureComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	int outputSamples = Super::OnGenerateAudio(OutAudio, NumSamples);

	if (outputSamples > 0 && CaptureReceiver)
	{
		uint8* arr = (uint8*)OutAudio;
		TArray<uint8> intsToFeed(arr, outputSamples * 4);
		CaptureReceiver->OnGenerateAudio(intsToFeed);
	}

	return outputSamples;
}

void UMPAudioCaptureComponent::SetCaptureReceiver(ACaptureReceiver* captureReceiver)
{
	CaptureReceiver = captureReceiver;
}
