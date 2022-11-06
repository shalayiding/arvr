// CSCI-715 Project: Bradley Klemick & Drew Haiber

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Particles/ParticleSystemComponent.h"
#include "FFTBins.h"
#include "Kismet/GameplayStatics.h"
#include "MPAudioCaptureComponent.h"
#include "AudioAnalyzerManager.h"
#include "MusicParticles1.generated.h"

class UAudioAnalyzerManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpawnParticleDelegate, int, noteIndex);

UCLASS()
class MLTEST2_API AMusicParticles1 : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMusicParticles1();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<float> NoteAmplitudes;

	UFUNCTION(BlueprintCallable)
	void SpawnParticleForNote(int noteIndex, FVector location, UParticleSystem* emitterTemplate);

	UPROPERTY(BlueprintAssignable)
	FSpawnParticleDelegate SpawnParticleDelegate;

	UFUNCTION()
	void OnGenerateAudio(const TArray<uint8>& bytes);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastReceiveAudio(const TArray<uint8>& bytes);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	int FFTSamples = 8192;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	int SampleRate = 48000;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	FString AudioFilename = FString();

private:
	UPROPERTY()
	TArray<UParticleSystemComponent*> ParticleSystems;

	UPROPERTY()
	int SinceLastBeat;

	// Product of two hours of struggle... without UPROPERTY, AAManager gets garbage collected!
	UPROPERTY()
	UAudioAnalyzerManager* AAManager;

	UPROPERTY()
	TArray<FFTBins> fftHist;

	UPROPERTY()
	bool IsServer;

private:
	int NoteToBin(float index);
	float GetSpectralCentroid(const TArray<float>& fftBins);
	float GetSpectralFlatness(const TArray<float>& fftBins);
	float GetSpectralDifference(const TArray<float>& fftBins);
	int GetSpectralDifferenceMaxIndex(const TArray<float>& fftBins);

};
