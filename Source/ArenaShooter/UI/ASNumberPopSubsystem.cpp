// Fill out your copyright notice in the Description page of Project Settings.


#include "ASNumberPopSubsystem.h"

#include "ASGameplayTags.h"
#include "ASUISettings.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Feedback/ASDamageMessage.h"

void UASNumberPopSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	const UASUISettings* Settings = GetDefault<UASUISettings>();
	ArrayName = Settings->NiagaraArrayName;
	PopSystem = Settings->NumberPopSystem.LoadSynchronous();
}

void UASNumberPopSubsystem::Deinitialize()
{
	ReleaseComponent();
	PopSystem = nullptr;
	
	Super::Deinitialize();
}

void UASNumberPopSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	ReleaseComponent();
}

void UASNumberPopSubsystem::AddNumberPop(const FASNumberPop& Pop)
{
	UNiagaraComponent* Comp = EnsureComponent();
	if (!Comp)
	{
		return;
	}
	
	const bool bSpecial = Pop.Tags.HasTag(FASGameplayTags::Gameplay_Zone_Head) || Pop.Tags.HasTag(FASGameplayTags::Damage_Lethal);
	const float Value = bSpecial ? static_cast<float>(-Pop.Damage) : static_cast<float>(Pop.Damage);
	
	TArray<FVector4> Pops = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector4(Comp, ArrayName);
	Pops.Emplace(Pop.Location.X, Pop.Location.Y, Pop.Location.Z, Value);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(Comp, ArrayName, Pops);
	
	Comp->SetWorldLocation(Pop.Location);
	Comp->Activate(false);
}

UNiagaraComponent* UASNumberPopSubsystem::EnsureComponent()
{
	if (!PopSystem)
	{
		return nullptr;
	}

	const ULocalPlayer* LP = GetLocalPlayer();
	UWorld* World = LP ? LP->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	
	if (UNiagaraComponent* Existing = NiagaraComp.Get())
	{
		if (Existing->GetWorld() == World)
		{
			return Existing;
		}
		ReleaseComponent();
	}
	
	AActor* Holder = World->GetWorldSettings();
	if (!Holder)
	{
		return nullptr;
	}

	UNiagaraComponent* Comp = NewObject<UNiagaraComponent>(Holder);
	Comp->SetAsset(PopSystem);
	Comp->bAutoActivate = false;
	Comp->bAutoActivate = false;
	Comp->SetAbsolute(true, true, true);
	Comp->RegisterComponentWithWorld(World);

	NiagaraComp = Comp;
	return Comp;
}

void UASNumberPopSubsystem::ReleaseComponent()
{
	if (UNiagaraComponent* Comp = NiagaraComp.Get())
	{
		Comp->DestroyComponent();
	}
	NiagaraComp.Reset();
}
