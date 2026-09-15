#include "ASGameplayEffectContext.h"

UScriptStruct* FASGameplayEffectContext::GetScriptStruct() const
{
	return StaticStruct();
}

FASGameplayEffectContext* FASGameplayEffectContext::Duplicate() const
{
	FASGameplayEffectContext* NewContext = new FASGameplayEffectContext();
	*NewContext = *this;
	if (GetHitResult())
	{
		NewContext->AddHitResult(*GetHitResult(), true);
	}
	
	return NewContext;
}

bool FASGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);
	TargetData.NetSerialize(Ar, Map, bOutSuccess);
	return true;
}
