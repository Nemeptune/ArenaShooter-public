// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LoadingProcessInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ASPreloadSubsystem.generated.h"

struct FStreamableHandle;
/**
 * Loads soft-referenced client assets at boot, keeps them for the session, and holds the
 * loading screen until they and the current map's PSOs are ready. 
 */
UCLASS()
class ARENASHOOTER_API UASPreloadSubsystem : public UGameInstanceSubsystem, public ILoadingProcessInterface
{
	GENERATED_BODY()
	
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
	
private:
	void HandleLoadingScreenVisibilityChanged(bool bVisible);
	void HandlePostLoadMap(UWorld* World);
	
	TSharedPtr<FStreamableHandle> PreLoadHandle;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle VisibilityChangedHandle;
	double MapLoadedTime = 0.0f;
	bool bWaitForPSOs = false;
};
