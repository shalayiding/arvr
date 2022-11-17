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

	bool isServer = UKismetSystemLibrary::IsServer(GetWorld());
	bool isWindows = UGameplayStatics::GetPlatformName() == "Windows";

	bool initSuccess;
	if (shouldPlayFile) {
		initSuccess = AAManager->InitPlayerAudio(AudioFilename);
	}
	else if (isServer)
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
	else if (isServer)
	{
		AAManager->StartCapture(false, true);
		UE_LOG(LogTemp, Display, TEXT("Started recording!"));

		auto udpComponent = NewObject<UUDPComponent>(this);
		udpComponent->Settings.bShouldAutoOpenSend = false;
		udpComponent->Settings.bShouldAutoOpenReceive = false;
		udpComponent->OnReceivedBytes.AddDynamic(this, &AMusicParticles1::OnUDPSubscriber);
		recvComponent = udpComponent;

		if (!udpComponent->OpenReceiveSocket("0.0.0.0", 57212))
		{
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Failed to open receive socket!")));
		}
	}
	else
	{
		AAManager->OpenStreamCapture(isWindows);
		UE_LOG(LogTemp, Display, TEXT("Started playback!"));

		auto udpComponent = NewObject<UUDPComponent>(this);
		udpComponent->Settings.bShouldAutoOpenSend = false;
		udpComponent->Settings.bShouldAutoOpenReceive = false;
		udpComponent->Settings.bShouldOpenReceiveToBoundSendPort = true;
		udpComponent->OnSendSocketOpened.AddDynamic(this, &AMusicParticles1::OnSendSocketOpened);
		udpComponent->OnReceivedBytes.AddDynamic(this, &AMusicParticles1::OnReceivedAudio);
		recvComponent = udpComponent;

		FString serverIP = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetServerNetworkAddress();
		if (!udpComponent->OpenSendSocket(serverIP, 57212))
		{
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Failed to open send socket!")));
		}
	}
}

void AMusicParticles1::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (recvComponent)
	{
		recvComponent->CloseReceiveSocket();
	}
}

// UDP Server functions

void AMusicParticles1::OnGenerateAudio(const TArray<uint8>& bytes)
{
	sendBuffer.Append(bytes);
	if (sendBuffer.Num() >= 4096)
	{
		for (auto comp : sendComponents)
		{
			comp->EmitBytes(sendBuffer);
		}
		sendBuffer.Empty();
	}
}

void AMusicParticles1::OnUDPSubscriber(const TArray<uint8>& bytes, const FString& ipAddress, int32 port)
{
	auto sendComponent = NewObject<UUDPComponent>(this);
	sendComponent->Settings.bShouldAutoOpenSend = false;
	sendComponent->Settings.bShouldAutoOpenReceive = false;

	sendComponents.Add(sendComponent);
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, FString::Printf(TEXT("Starting audio output to %s:%d"), *ipAddress, port));
	sendComponent->OpenSendSocket(ipAddress, port);
}

// UDP Client functions

void AMusicParticles1::OnSendSocketOpened(int32 specifiedPort, int32 boundPort)
{
	TArray<TSharedPtr<FInternetAddr>> addrs;
	ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalAdapterAddresses(addrs);
	FString theIP = "";
	for (TSharedPtr<FInternetAddr> addr : addrs)
	{
		FString bindableIP = addr.Get()->ToString(false);
		if (bindableIP.StartsWith("129"))
		{
			theIP = bindableIP;
		}
	}

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White,
			FString::Printf(TEXT("Opening receive socket %s:%d"), *theIP, boundPort));
	if (!recvComponent || !recvComponent->OpenReceiveSocket(theIP, boundPort))
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red,
				FString::Printf(TEXT("Failed to open receive socket! %s:%d"), *theIP, boundPort));
	}

	// Tell server which port was chosen (no longer used)
	uint8* portBytes = (uint8*)&boundPort;
	TArray<uint8> portArray(portBytes, 4);
	recvComponent->EmitBytes(portArray);
}

void AMusicParticles1::OnReceivedAudio(const TArray<uint8>& bytes, const FString& ipAddress, int32 port)
{
	AAManager->FeedStreamCapture(bytes);
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
	auto particleComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), emitterTemplate, spawnTransform);
	ParticleSystems[noteIndex] = particleComponent;

	float dim = sqrtf(NoteAmplitudes[noteIndex]) / 3.0f;
	FVector sizeVector = FVector(dim, dim, dim);
	float hue = (noteIndex % 12) * 30.0f;
	float yMod = FMath::RandRange(-2.0f, 2.0f);
	float zMod = FMath::RandRange(0.0f, 2.0f);
	auto veloVector = FVector(noteIndex * -12.0f, noteIndex * yMod, noteIndex * zMod);

	particleComponent->SetColorParameter("color", UKismetMathLibrary::HSVToRGB(hue, 0.8f, 0.2f));
	particleComponent->SetVectorParameter("size", sizeVector);
	particleComponent->SetVectorParameter("velo", veloVector);
	particleComponent->ResetBurstLists();
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




