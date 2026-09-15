#include "Weapon/ASWeaponDefinition.h"

#if WITH_EDITOR
#include "ASWeaponCosmetic.h"
#include "ASWeaponInstance.h"
#include "Misc/DataValidation.h"

EDataValidationResult UASWeaponDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (!FPLinkedLayer || !TPLinkedLayer)
	{
		Context.AddError(FText::FromString(TEXT("Weapon definition is missing FPLinkedLayer or TPLinkedLayer — the character will animate unarmed.")));
		Result = EDataValidationResult::Invalid;
	}
	if (!CosmeticClass)
	{
		Context.AddError(FText::FromString(TEXT("No CosmeticClass — no mesh, and muzzle transforms fall back to the world origin."))); Result = EDataValidationResult::Invalid;
	}
	if (!InstanceClass)
	{
		Context.AddError(FText::FromString(TEXT("No InstanceClass — behaviour subclasses (e.g. rocket launcher) will not run."))); Result = EDataValidationResult::Invalid;
	}
	if (!AbilitySet)
	{
		Context.AddError(FText::FromString(TEXT("No AbilitySet — this weapon grants nothing and cannot fire."))); Result = EDataValidationResult::Invalid;
	}
	if (FireInterval <= 0.f)
	{
		Context.AddWarning(FText::FromString(TEXT("FireInterval is 0 — no refire limit.")));
	}
	if (!AmmoType.IsValid())
	{
		Context.AddError(FText::FromString(TEXT("No AmmoType — no ammo pickup can ever refill this weapon.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif