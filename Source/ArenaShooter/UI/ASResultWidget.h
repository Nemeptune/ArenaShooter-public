// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASCommonActivatableWidget.h"
#include "ASResultWidget.generated.h"

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASResultWidget : public UASCommonActivatableWidget
{
	GENERATED_BODY()
public:
	void InitResult(bool bWon, bool bDraw);

protected:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnResultSet(bool bWon, bool bDraw);

	UFUNCTION(BlueprintCallable)
	void ReturnToMainMenu();

	UPROPERTY(EditDefaultsOnly, Category = "AS|Result")
	TSoftObjectPtr<UWorld> MainMenuLevel;
};
