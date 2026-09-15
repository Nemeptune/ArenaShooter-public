// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ASNumberPopSubsystem.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
struct FASNumberPop;
/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASNumberPopSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;
	
	void AddNumberPop(const FASNumberPop& Pop);

private:
	UNiagaraComponent* EnsureComponent();
	void ReleaseComponent();
	
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> PopSystem;
	
	FName ArrayName;
	
	// Outered to the current PlayerController
	TWeakObjectPtr<UNiagaraComponent> NiagaraComp;
};
