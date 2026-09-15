#pragma once
#include "GameplayEffectTypes.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "ASGameplayEffectContext.generated.h"

USTRUCT()
struct FASGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()
	
public:
	FGameplayAbilityTargetDataHandle TargetData;

	virtual UScriptStruct* GetScriptStruct() const override;

	virtual FASGameplayEffectContext* Duplicate() const override;

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

template<>
struct TStructOpsTypeTraits<FASGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FASGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};