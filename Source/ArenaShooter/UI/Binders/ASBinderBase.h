// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ASBinderBase.generated.h"

UCLASS(Abstract)
class ARENASHOOTER_API UASBinderBase : public UObject
{
	GENERATED_BODY()
	
public:
	
	void Activate();
	void Shutdown();
	bool IsActive() const {return bActive;}
	
protected:
	virtual void SubscribeAll() PURE_VIRTUAL(UBinderBase::SubscibeAll,);
	virtual void UnSubscribeAll() PURE_VIRTUAL(UBinderBase::UnSubscibeAll,);
	virtual void RefreshAll() PURE_VIRTUAL(UBinderBase::RefreshAll,);
	
private:
	bool bActive = false;
};
