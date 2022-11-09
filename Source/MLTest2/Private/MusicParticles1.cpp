// CSCI-715 Project: Bradley Klemick & Drew Haiber


#include "MusicParticles1.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"

constexpr int START_NOTE = -29;
constexpr int END_NOTE = 43;

// Sets default values
AMusicParticles1::AMusicParticles1()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;

	for (int note = START_NOTE; note < END_NOTE; note++)
	{
		ParticleSystems.Add(nullptr);
		NoteAmplitudes.Add(0.0f);
	}
}

// Called when the game starts or when spawned
void AMusicParticles1::BeginPlay()
{
	SinceLastBeat = 0;
	Super::BeginPlay();
	bool shouldPlayFile = AudioFilename.Len() > 0;

	AAManager = NewObject<UAudioAnalyzerManager>(this, TEXT("AAManager"));

	IsServer = UKismetSystemLibrary::IsServer(GetWorld());
	bool isWindows = UGameplayStatics::GetPlatformName() == "Windows";
	isWindows = false;

	bool initSuccess;
	if (shouldPlayFile) {
		initSuccess = AAManager->InitPlayerAudio(AudioFilename);
	}
	else if (IsServer)
	{
		initSuccess = AAManager->InitCapturerAudioEx(SampleRate, EAudioDepth::B_16, EAudioFormat::Signed_Int, 1.0f);
		AAManager->OnCapturedData.AddDynamic(this, &AMusicParticles1::OnGenerateAudio);
	}
	else
	{
		initSuccess = AAManager->InitStreamAudio(
			1,
			SampleRate,
			EAudioDepth::B_16,
			EAudioFormat::Signed_Int,
			1.0f,
			isWindows);
	}

	if (!initSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to open file! %s"), *AudioFilename);
		return;
	}

	float timeWindow = FFTSamples / (float)SampleRate;
	AAManager->InitSpectrumConfig(
		ESpectrumType::ST_Linear,
		EChannelSelectionMode::All_in_one,
		0,
		FFTSamples,
		timeWindow,
		30,
		true,
		1);

	if (shouldPlayFile)
	{
		AAManager->Play();
		UE_LOG(LogTemp, Display, TEXT("Playing local test file!"));
	}
	else if (IsServer)
	{
		AAManager->StartCapture(false, true);
		UE_LOG(LogTemp, Display, TEXT("Started recording!"));
	}
	else
	{
		AAManager->OpenStreamCapture(isWindows);
		UE_LOG(LogTemp, Display, TEXT("Started playback!"));
	}
}

void AMusicParticles1::OnGenerateAudio(const TArray<uint8>& bytes)
{
	MulticastReceiveAudio(bytes);
}

void AMusicParticles1::MulticastReceiveAudio_Implementation(const TArray<uint8>& bytes)
{
	if (!IsServer)
	{
		AAManager->FeedStreamCapture(bytes);
	}
}

// Called every frame
void AMusicParticles1::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!AAManager || !(AAManager->IsStreamCapturing() || AAManager->IsCapturing() || AAManager->IsPlaying()))
	{
		return;
	}

	TArray<float> frequencies;
	TArray<float> avgFrequencies;
	AAManager->GetSpectrum(frequencies, avgFrequencies, false);

	if (frequencies.Num() == 0)
	{
		return;
	}

	for (int noteIndex = START_NOTE; noteIndex < END_NOTE; noteIndex++)
	{
		float ampAccum = 0.0f;
		int lowBin = NoteToBin((float)noteIndex - 0.5f);
		int highBin = NoteToBin((float)noteIndex + 0.5f);
		for (int bin = lowBin; bin < highBin; bin++)
		{
			ampAccum = std::max(ampAccum, frequencies[bin]);
		}

		NoteAmplitudes[noteIndex - START_NOTE] = (ampAccum * 20.0f / (highBin - lowBin));
	}

	float spectralDiff = GetSpectralDifference(NoteAmplitudes);
}

int AMusicParticles1::NoteToBin(float index)
{
	float noteFreq = powf(2.0, index / 12.0f) * 440.0f;
	return (int)(noteFreq / (SampleRate / (float)FFTSamples));
}

// Adapted from AudioAnalyzerTools CoreFrequencyDomainFeatures.cpp
float AMusicParticles1::GetSpectralCentroid(const TArray<float>& fftBins)
{
	float sumAmplitudes = 0.0f;
	float sumWeightedAmplitudes = 0.0f;
	for (int bin = 0; bin < FFTSamples; bin++)
	{
		sumAmplitudes += fftBins[bin];
		sumWeightedAmplitudes += fftBins[bin] * bin;
	}

	return sumAmplitudes > 0.0f ? sumWeightedAmplitudes / sumAmplitudes : 0.0f;
}

float AMusicParticles1::GetSpectralFlatness(const TArray<float>& fftBins)
{
	float sumValue = 0.0f;
	float logSumValue = 0.0f;
	for (int bin = 0; bin < FFTSamples; bin++)
	{
		float val = fftBins[bin];
		sumValue += val;
		logSumValue += logf(val);
	}

	sumValue /= FFTSamples;
	logSumValue /= FFTSamples;

	return sumValue > 0.0f ? expf(logSumValue) / sumValue : 0.0f;
}

float AMusicParticles1::GetSpectralDifference(const TArray<float>& fftBins)
{
	FFTBins* fftStruct = new FFTBins();
	fftStruct->bins = fftBins;

	int histLen = fftHist.Num();
	fftHist.Add(*fftStruct);

	if (histLen == 0)
	{
		return 0.0f;
	}

	float accum = 0.0f;
	for (int bin = 0; bin < fftBins.Num(); bin++)
	{
		float histTotal = 0.0f;
		for (int age = 0; age < histLen; age++)
		{
			histTotal += fftHist[age].bins[bin];
		}
		float histAverage = histTotal / histLen;

		UParticleSystemComponent* existSystem = ParticleSystems[bin];

		// Peak detection: if neighbor is greater, ignore this one
		if ((bin == 0 || fftBins[bin - 1] > fftBins[bin]) || (bin < fftBins.Num() - 1 && fftBins[bin + 1] > fftBins[bin]))
		{
			continue;
		}

		if (existSystem && fftBins[bin] < 0.01f)
		{
			existSystem->DestroyComponent();
			ParticleSystems[bin] = nullptr;
		}

		float diff = fftBins[bin] - histAverage;
		if (diff > 0.02f)
		{
			float dim = sqrtf(fftBins[bin]) / 2.0f;
			FVector sizeVector = FVector(dim, dim, dim);
			if (existSystem)
			{
				FVector oldSizeVector;
				existSystem->GetVectorParameter("size", oldSizeVector);
				if (oldSizeVector.X < dim) {
					existSystem->SetVectorParameter("size", sizeVector);
				}
			}
			else
			{
				SpawnParticleDelegate.Broadcast(bin);
			}
		}
	}

	if (fftHist.Num() > 10)
	{
		fftHist.RemoveAt(0, 1, false);
	}

	return accum;
}

void AMusicParticles1::SpawnParticleForNote(int noteIndex, FVector location, UParticleSystem* emitterTemplate)
{
	// Ensure player is close enough to avoid spawning particles on different stages
	APawn* pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (pawn && FVector::Dist(pawn->GetActorLocation(), location) > 2000.0f)
	{
		return;
	}

	auto spawnTransform = FTransform(location);
	float dim = sqrtf(NoteAmplitudes[noteIndex]) / 2.0f;
	FVector sizeVector = FVector(dim, dim, dim);
	auto particleComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), emitterTemplate, spawnTransform);
	float hue = (noteIndex % 12) * 30.0f;
	auto veloVector = FVector(noteIndex * -15.0f, 0.0f, 0.0f);
	particleComponent->SetColorParameter("color", UKismetMathLibrary::HSVToRGB(hue, 0.8f, 0.2f));
	particleComponent->SetVectorParameter("size", sizeVector);
	particleComponent->SetVectorParameter("velo", veloVector);
	ParticleSystems[noteIndex] = particleComponent;
}

int AMusicParticles1::GetSpectralDifferenceMaxIndex(const TArray<float>& fftBins) {
	FFTBins* fftStruct = new FFTBins();
	fftStruct->bins = fftBins;

	int histLen = fftHist.Num();
	fftHist.Add(*fftStruct);

	if (histLen == 0)
	{
		return 0.0f;
	}
	int maxInd = -1;
	float maxVal = 0;
	for (int bin = 0; bin < fftBins.Num(); bin++) {
		float cval = fftBins[bin];
		float pval = fftHist[histLen - 1].bins[bin];
		float dif = cval;
		if ((dif / cval) > .75 && dif > .007 && dif > maxVal){
			maxInd = bin;
			maxVal = dif;
		}
	}

	if (fftHist.Num() > 10)
	{
		fftHist.RemoveAt(0, 1, false);
	}


	return maxInd;
}




