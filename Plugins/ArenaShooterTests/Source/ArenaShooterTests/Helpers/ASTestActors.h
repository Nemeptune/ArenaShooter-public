// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Character/ASCharacterMovementComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "ASTestActors.generated.h"

class UASCombatAttributeSet;
class UAbilitySystemComponent;

UCLASS()
class ARENASHOOTERTESTS_API AASTestAbilityActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AASTestAbilityActor();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UASCombatAttributeSet> Attributes;
};

UCLASS()
class UASTestMovementComponent : public UASCharacterMovementComponent
{
	GENERATED_BODY()

public:
	FVector GetPendingImpulse() const;
};

UCLASS()
class AASTestCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AASTestCharacter(const FObjectInitializer& ObjectInitializer);
	
	UASTestMovementComponent* GetTestMovementComponent() const;
};

UCLASS()
class AASTestController : public AController
{
	GENERATED_BODY()
};
