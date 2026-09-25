// Fill out your copyright notice in the Description page of Project Settings.


#include "ASPreloadSubsystem.h"

#include "AbilitySystemGlobals.h"
#include "EngineUtils.h"
#include "LoadingScreenManager.h"
#include "PipelineStateCache.h"
#include "Components/SkyLightComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/SkyLight.h"
#include "UI/ASUIConfig.h"
#include "UI/ASUISettings.h"

namespace ASPreLoad
{
	constexpr double MaxPSOWaitSeconds = 10.0;
}

bool UASPreloadSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !CastChecked<UGameInstance>(Outer)->IsDedicatedServerInstance();
}

void UASPreloadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	ULoadingScreenManager* LoadingScreen = Collection.InitializeDependency<ULoadingScreenManager>();
	
	Super::Initialize(Collection);
	
	UAbilitySystemGlobals::Get();
	
	TArray<FSoftObjectPath> Paths;
	if (const UASUIConfig* UIConfig = GetDefault<UASUISettings>()->UIConfig.LoadSynchronous())
	{
		Paths.Add(UIConfig->HUDWidgetClass.ToSoftObjectPath());
		Paths.Add(UIConfig->ResultWidgetClass.ToSoftObjectPath());
		Paths.Add(UIConfig->GameMenuClass.ToSoftObjectPath());
	}
	Paths.RemoveAll([](const FSoftObjectPath& Path)
	{
		return Path.IsNull();
	});
	
	if (!Paths.IsEmpty())
	{
		PreLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(Paths));
	}
	
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UASPreloadSubsystem::HandlePostLoadMap);
	
	if (LoadingScreen)
	{
		LoadingScreen->RegisterLoadingProcessor(this);
		VisibilityChangedHandle = LoadingScreen->OnLoadingScreenVisibilityChangedDelegate().AddUObject(this, &ThisClass::HandleLoadingScreenVisibilityChanged);
	}
}

void UASPreloadSubsystem::Deinitialize()
{
	if (ULoadingScreenManager* LoadingScreen = GetGameInstance()->GetSubsystem<ULoadingScreenManager>())
	{
		LoadingScreen->OnLoadingScreenVisibilityChangedDelegate().Remove(VisibilityChangedHandle);
		LoadingScreen->UnregisterLoadingProcessor(this);
	}
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	
	if (PreLoadHandle.IsValid())
	{
		PreLoadHandle->ReleaseHandle();
		PreLoadHandle.Reset();
	}
	
	Super::Deinitialize();
}

bool UASPreloadSubsystem::ShouldShowLoadingScreen(FString& OutReason) const
{
	if (PreLoadHandle.IsValid() && PreLoadHandle->IsLoadingInProgress())
	{
		OutReason = TEXT("Preloading UI Classes");
		return true;
	}
	
	if (bWaitForPSOs && FPlatformTime::Seconds() - MapLoadedTime < ASPreLoad::MaxPSOWaitSeconds && PipelineStateCache::IsPrecaching())
	{
		OutReason = FString::Printf(TEXT("Precaching PSOs (%u requests)"), PipelineStateCache::NumActivePrecacheRequests());
		return true;
	}
	
	return false;
}

void UASPreloadSubsystem::HandleLoadingScreenVisibilityChanged(bool bVisible)
{
	if (bVisible)
	{
		return;
	}

	bWaitForPSOs = false;

	// The engine's own sky capture ran on the first tick after LoadMap, before PSOs finished and
	// while PSO-delayed primitives had no scene proxy. Everything is drawable now.
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		for (TActorIterator<ASkyLight> It(World); It; ++It)
		{
			if (USkyLightComponent* SkyLight = It->GetLightComponent())
			{
				SkyLight->RecaptureSky();
			}
		}
	}
}

void UASPreloadSubsystem::HandlePostLoadMap(UWorld* World)
{
	if (World && World->GetGameInstance() == GetGameInstance())
	{
		MapLoadedTime = FPlatformTime::Seconds();
		bWaitForPSOs = true;
	}
}
