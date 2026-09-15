// Fill out your copyright notice in the Description page of Project Settings.


#include "ASNumberPopRelay.h"

#include "System/ASGameplayTags.h"
#include "System/ASLogChannels.h"
#include "Messages/ASMessageTags.h"
#include "GameFramework/PlayerState.h"
#include "Player/ASPlayerController.h"

void UASNumberPopRelay::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	if (InWorld.GetNetMode() == NM_Client)
	{
		return;
	}
	
	ListenerHandle = UGameplayMessageSubsystem::Get(&InWorld).RegisterListener(FASMessageTags::Damage_Dealt, this, &UASNumberPopRelay::OnDamageDealt);
}

void UASNumberPopRelay::Deinitialize()
{
	if (ListenerHandle.IsValid())
	{
		ListenerHandle.Unregister();
	}
	
	Super::Deinitialize();
}

void UASNumberPopRelay::OnDamageDealt(FGameplayTag Channel, const FASDamageDealtMessage& Message)
{
	if (!Message.Instigator)
	{
		return;
	}
	
	APlayerController* PC = Cast<APlayerController>(Message.Instigator->GetOwner());
	if (!PC || Message.Target == PC->GetPawn())
	{
		return;
	}
	
	const FGameplayTagContainer ZoneFilter(FASGameplayTags::Gameplay_Zone);
	const FGameplayTagContainer NewZone = Message.Tags.Filter(ZoneFilter);
	
	TArray<FPendingPop>& Batch = Pending.FindOrAdd(PC);
	for (FPendingPop& Existing : Batch)
	{
		if (Existing.Target == Message.Target && Existing.Tags == Message.Tags)
		{
			Existing.Damage += Message.Damage;
			Existing.Tags.AppendTags(Message.Tags);
			return;
		}
	}
	
	FPendingPop& New = Batch.AddDefaulted_GetRef();
	New.Target = Message.Target;
	New.Location = Message.Location;
	New.Damage = Message.Damage;
	New.Tags = Message.Tags;
	
	if (!bFlushScheduled)
	{
		bFlushScheduled = true;
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UASNumberPopRelay::Flush);
	}
}

void UASNumberPopRelay::Flush()
{
	bFlushScheduled = false;
	
	for (TPair<TWeakObjectPtr<APlayerController>, TArray<FPendingPop>>& Pair : Pending)
	{
		AASPlayerController* PC = Cast<AASPlayerController>(Pair.Key.Get());
		if (!PC)
		{
			continue;
		}
		
		TArray<FASNumberPop> Wire;
		Wire.Reserve(Pair.Value.Num());
		for (const FPendingPop& Entry : Pair.Value)
		{
			FASNumberPop& Pop = Wire.AddDefaulted_GetRef();
			Pop.Location = Entry.Location;
			Pop.Damage = static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(Entry.Damage), 0, static_cast<int32>(MAX_uint16)));
			Pop.Tags = Entry.Tags;
		}
		PC->ClientNumberPops(Wire);
	}
	Pending.Reset();
}
