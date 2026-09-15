// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASPlayerControllerBase.h"
#include "ASMenuPlayerController.generated.h"

class UCommonActivatableWidget;

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API AASMenuPlayerController : public AASPlayerControllerBase
{
	GENERATED_BODY()
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UCommonActivatableWidget> MenuClass;
	UPROPERTY()
	TObjectPtr<UCommonActivatableWidget> MenuWidget;
};
