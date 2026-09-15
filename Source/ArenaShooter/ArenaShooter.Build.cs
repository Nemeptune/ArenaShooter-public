// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ArenaShooter : ModuleRules
{
	public ArenaShooter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(new string[] { "ArenaShooter" });
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AnimGraphRuntime",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"Niagara",
			"PhysicsCore",
			"NetCore",
			"ModelViewViewModel",
			"FieldNotification",
			"SlateCore",
			"Slate",
			"UMG",
			"MultiplayerSessions",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"OnlineSubsystemNull",
			"CommonUI",
			"CommonInput",
			"GameSettings",
			"DeveloperSettings",
			"ApplicationCore",
			"AudioModulation",
			"GameplayMessageRuntime",
			"CommonLoadingScreen",
			"MetasoundEngine",
			"RHI"
		});
		SetupIrisSupport(Target);
		
		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Mac || Target.Platform == UnrealTargetPlatform.Linux)
		{
			PublicDefinitions.Add("AS_WITH_STEAM_AVATARS=1");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");
		}
		else
		{
			PublicDefinitions.Add("AS_WITH_STEAM_AVATARS=0");
		}
	}
}
