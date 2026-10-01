#pragma once

#include "CoreMinimal.h"

class UGameplayEffect;

namespace ASTestEffects
{
	/** An instant effect that adds Amount straight to the Damage meta attribute, skipping the execution. */
	UGameplayEffect* MakeRawDamage(float Amount);

	/** An instant effect that runs UASDamageExecution on its SetByCaller Data.Damage, as weapon damage does. */
	UGameplayEffect* MakeExecutionDamage();
}