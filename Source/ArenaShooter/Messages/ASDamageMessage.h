#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ASDamageMessage.generated.h"

class APlayerState;

USTRUCT(BlueprintType)
struct FASDamageDealtMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	TObjectPtr<APlayerState> Instigator = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	float Damage = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FGameplayTagContainer Tags;
};

USTRUCT()
struct FASNumberPop
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize Location = FVector::ZeroVector;

	UPROPERTY()
	uint16 Damage = 0;

	UPROPERTY()
	FGameplayTagContainer Tags;
};