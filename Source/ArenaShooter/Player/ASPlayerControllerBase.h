// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LoadingProcessInterface.h"
#include "ASPlayerControllerBase.generated.h"

UCLASS(Abstract)
class ARENASHOOTER_API AASPlayerControllerBase : public APlayerController, public ILoadingProcessInterface
{
	GENERATED_BODY()
	
public:
	virtual void ReceivedPlayer() override;
	virtual void OnRep_PlayerState() override;
	
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
};
