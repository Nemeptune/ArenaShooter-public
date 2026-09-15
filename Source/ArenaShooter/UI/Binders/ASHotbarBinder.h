// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASBinderBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ASHotbarBinder.generated.h"

struct FASActiveSlotChangedMessage;
struct FASSlotChangedMessage;
class UASWeaponSlotViewModel;
class AASCharacter;
struct FOnAttributeChangeData;
class UASHUDWidget;
class AASPlayerController;
class UASAbilitySystemComponent;

/**
 * HUD weapon slots
 */
UCLASS()
class ARENASHOOTER_API UASHotbarBinder : public UASBinderBase
{
	GENERATED_BODY()

public:
	void Init(AASPlayerController* PC, UASHUDWidget* InHUDWidget);
	
protected:
	virtual void SubscribeAll() override;
	virtual void UnSubscribeAll() override;
	virtual void RefreshAll() override;

private:
	void EnsureSlots(int32 NumSlots);

	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);
	
	void OnSlotChanged(FGameplayTag, const FASSlotChangedMessage& Message);
	void ApplySlot(const FASSlotChangedMessage& Message);
	
	void OnActiveSlotChanged(FGameplayTag Channel, const FASActiveSlotChangedMessage& Message);
	void ApplyActiveSlot(int32 NewSlot);

	UPROPERTY()
	TObjectPtr<UASHUDWidget> HUDWidget;
	UPROPERTY()
	TArray<TObjectPtr<UASWeaponSlotViewModel>> SlotsVM;
	UPROPERTY()
	TObjectPtr<UASWeaponSlotViewModel> ActiveSlotVM;
	
	TWeakObjectPtr<AASCharacter> LocalCharacter;
	TWeakObjectPtr<AASPlayerController> BoundPC;
	TWeakObjectPtr<APlayerState> BoundPS;
	
	FGameplayMessageListenerHandle SlotChangedHandle;
	FGameplayMessageListenerHandle ActiveSlotChangedHandle;
};
