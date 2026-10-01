// Fill out your copyright notice in the Description page of Project Settings.


#include "ASTestActors.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ASCombatAttributeSet.h"


AASTestAbilityActor::AASTestAbilityActor()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	Attributes = CreateDefaultSubobject<UASCombatAttributeSet>("AttributeSet");
}

UAbilitySystemComponent* AASTestAbilityActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

AASTestCharacter::AASTestCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UASTestMovementComponent>(ACharacter::CharacterMovementComponentName))
{
}

FVector UASTestMovementComponent::GetPendingImpulse() const
{
	return PendingImpulseToApply;
}

UASTestMovementComponent* AASTestCharacter::GetTestMovementComponent() const
{
	return CastChecked<UASTestMovementComponent>(GetCharacterMovement());
}

