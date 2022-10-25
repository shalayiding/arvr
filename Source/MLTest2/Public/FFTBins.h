// CSCI-715 Project: Bradley Klemick & Drew Haiber

#pragma once

#include "CoreMinimal.h"
#include "Particles/ParticleSystemComponent.h"
#include "FFTBins.generated.h"

USTRUCT()
struct FFTBins
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<float> bins;
};
