// Fill out your copyright notice in the Description page of Project Settings.


#include "ASBinderBase.h"

void UASBinderBase::Activate()
{
	if (!bActive)
	{
		bActive = true;
		SubscribeAll();
	}
	RefreshAll();
}

void UASBinderBase::Shutdown()
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	UnSubscribeAll();
}
