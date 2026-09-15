// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GameplayCues/ASGameplayCueNotify_BeamActor.h"

#include "System/ASLogChannels.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/ASWeaponInstance.h"

AASGameplayCueNotify_BeamActor::AASGameplayCueNotify_BeamActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	
	bAutoDestroyOnRemove = true;
}

bool AASGameplayCueNotify_BeamActor::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (!MyTarget || MyTarget->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}
	
	if (FireAudioComp)
	{
		FireAudioComp->Stop();
		FireAudioComp = nullptr;
	}
	if (BeamComp)
	{
		BeamComp->DestroyComponent();
		BeamComp = nullptr;
	}

	TargetPawn = Cast<APawn>(MyTarget);

	const UASWeaponInstance* Weapon = Cast<UASWeaponInstance>(Parameters.SourceObject.Get());
	if (!Weapon || !BeamSystem)
	{
		return false;
	}

	const bool bFirstPerson = TargetPawn.IsValid() && TargetPawn->IsLocallyControlled();
	USkeletalMeshComponent* WeaponMesh = bFirstPerson ? Weapon->GetWeaponMesh1P() :  Weapon->GetWeaponMesh3P();
	if (!WeaponMesh)
	{
		return false;
	}

	BeamComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		BeamSystem, WeaponMesh, MuzzleSocketName,
		FVector::ZeroVector, FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget, false);

	if (bFirstPerson)
	{
		BeamComp->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);	
	}
	
	WeaponMeshWeak = WeaponMesh;
	ActiveTime = 0.f;

	if (FireStartSound)
	{
		UGameplayStatics::SpawnSoundAttached(FireStartSound, WeaponMesh, MuzzleSocketName,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	}

	if (USoundBase* LoopSound = bFirstPerson ? FireLoopSound1P : FireLoopSound3P)
	{
		FireAudioComp = UGameplayStatics::SpawnSoundAttached(LoopSound, WeaponMesh, MuzzleSocketName,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget,
			true, 1.f, 1.f, 0.f, nullptr, nullptr, true);

		if (FireAudioComp)
		{
			FireAudioComp->SetFloatParameter(IntensityParamName, 0.f);
		}
	}

	SetActorTickEnabled(BeamComp != nullptr);
	return true;
}

void AASGameplayCueNotify_BeamActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!BeamComp)
	{
		return;
	}
	
	if (FireAudioComp)
	{
		ActiveTime += DeltaTime;
		const float Intensity = IntensityRampTime > 0.f ? FMath::Clamp(ActiveTime / IntensityRampTime, 0.f, 1.f) : 1.f;
		FireAudioComp->SetFloatParameter(IntensityParamName, Intensity);
	}

	BeamComp->SetVariablePosition(BeamEndParamName, ComputeBeamEnd());
}

FVector AASGameplayCueNotify_BeamActor::ComputeBeamEnd() const
{
	APawn* Pawn = TargetPawn.Get();
	if (!Pawn)
	{
		return BeamComp->GetComponentLocation();
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	if (AController* Controller = Pawn->GetController())
	{
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		ViewLocation = Pawn->GetPawnViewLocation();
		ViewRotation = Pawn->GetBaseAimRotation();
	}
	
	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + ViewRotation.Vector() * TraceRange;

	UWorld* World = GetWorld();
	if (!World)
	{
		return TraceEnd;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BeamVisualTrace), true, Pawn);
	Params.AddIgnoredActor(Pawn);

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannel, Params))
	{
		return Hit.ImpactPoint;
	}
	
	return TraceEnd;
}

bool AASGameplayCueNotify_BeamActor::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	SetActorTickEnabled(false);

	if (BeamComp)
	{
		BeamComp->Deactivate();
		BeamComp->DestroyComponent();
		BeamComp = nullptr;
	}
	if (FireAudioComp)
	{
		FireAudioComp->FadeOut(StopFadeTime, 0.f);
		FireAudioComp = nullptr;
	}

	if (FireStopSound)
	{
		if (USkeletalMeshComponent* Mesh = WeaponMeshWeak.Get())
		{
			UGameplayStatics::SpawnSoundAttached(FireStopSound, Mesh, MuzzleSocketName,
				FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		}
	}
	WeaponMeshWeak = nullptr;
	TargetPawn = nullptr;
	return true;
}