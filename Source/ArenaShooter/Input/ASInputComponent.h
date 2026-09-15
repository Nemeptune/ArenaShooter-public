// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASInputConfig.h"
#include "EnhancedInputComponent.h"
#include "ASInputComponent.generated.h"

class UASInputConfig;

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()
	
public:
	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UASInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc);
};

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UASInputComponent::BindAbilityActions(const UASInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc)
{
	check(InputConfig);

	for (const FASInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag);
			}

			if (ReleasedFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag);
			}
		}
	}
}
