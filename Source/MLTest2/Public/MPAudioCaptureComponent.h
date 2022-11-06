// RIT CSCI-715 Semester 2221 Project

#pragma once

#include "CoreMinimal.h"
#include "AudioCaptureComponent.h"
#include "CaptureReceiver.h"
#include "MPAudioCaptureComponent.generated.h"

/**
 * Override Unreal's built in AudioCaptureComponent, which is implemented by
 * the Magic Leap API, to direct its input to AudioAnalyzer.
 */
UCLASS()
class MLTEST2_API UMPAudioCaptureComponent : public UAudioCaptureComponent
{
	GENERATED_BODY()

public:
	void SetCaptureReceiver(ACaptureReceiver* captureReceiver);
	
protected:
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	ACaptureReceiver* CaptureReceiver;
};
