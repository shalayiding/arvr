// CSCI-715 Project: Bradley Klemick & Drew Haiber

#pragma once

#include "CoreMinimal.h"

class ACaptureReceiver
{
public:
	virtual void OnGenerateAudio(const TArray<uint8>& bytes) = 0;
};
