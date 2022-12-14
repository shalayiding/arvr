// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class MLTest2 : ModuleRules
{
	public MLTest2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore",
			"AudioCapture", "AudioMixer", "Sockets" });
		
		PrivateDependencyModuleNames.AddRange(new string[] { "ParallelcubeTaglib", "ParallelcubeAudioAnalyzer",
			"UDPWrapper"});

		PrivateIncludePaths.Add("../Plugins/Marketplace/AudioAnalyzer/Source/AudioAnalyzer/Private");
		PrivateIncludePaths.Add("../Plugins/Marketplace/AudioAnalyzer/Source/AudioAnalyzer/Thirdparty/KissFFT_130");
		PrivateIncludePaths.Add("../Plugins/Marketplace/AudioAnalyzer/Source/AudioAnalyzer/Thirdparty/KissFFT_130/tools");
		PrivateIncludePaths.Add("../Plugins/Marketplace/AudioAnalyzer/Source/Thirdparty/miniaudio/include");
		PrivateIncludePaths.Add("../Plugins/Marketplace/AudioAnalyzer/Source/AudioAnalyzer/Thirdparty/stb");

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
