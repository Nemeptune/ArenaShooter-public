// Fill out your copyright notice in the Description page of Project Settings.


#include "ASGameplayCueNotify_WeaponFireActor.h"

#include "AbilitySystem/ASGameplayEffectContext.h"
#include "System/ASLogChannels.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "FX/ImpactStatics.h"
#include "Kismet/GameplayStatics.h"
#include "System/ASProfiling.h"
#include "Weapon/ASWeaponInstance.h"

AASGameplayCueNotify_WeaponFireActor::AASGameplayCueNotify_WeaponFireActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bAutoDestroyOnRemove = false;
	bAutoAttachToOwner = false;
	bUniqueInstancePerInstigator = false;
	bUniqueInstancePerSourceObject = false;

	NumPreallocatedInstances = 3;
}

bool AASGameplayCueNotify_WeaponFireActor::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AASGameplayCueNotify_WeaponFireActor::OnExecute_Implementation);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_FireCue);
	
	if (!MyTarget || MyTarget->GetNetMode() == NM_DedicatedServer)
	{
		UE_LOG(LogAS, Warning, TEXT("FireCue: no target, or dedicated server."));
		return false;
	}

	const UASWeaponInstance* Weapon = Cast<UASWeaponInstance>(Parameters.SourceObject.Get());
	if (!Weapon)
	{
		UE_LOG(LogAS, Warning, TEXT("FireCue on %s: SourceObject did not resolve to a weapon (raw=%s)."),
			*GetNameSafe(MyTarget), *GetNameSafe(Parameters.SourceObject.Get()));
		return false;
	}

	const APawn* Pawn = Cast<APawn>(MyTarget);
	const bool bFirstPerson = Pawn && Pawn->IsLocallyControlled();
	USoundBase* FireSound = bFirstPerson ? FireSound1P : FireSound3P;
	USkeletalMeshComponent* WeaponMesh = bFirstPerson ? Weapon->GetWeaponMesh1P() : Weapon->GetWeaponMesh3P();
	if (!WeaponMesh)
	{
		UE_LOG(LogAS, Warning, TEXT("FireCue on %s: no %s mesh on weapon %s."),
			*GetNameSafe(MyTarget), bFirstPerson ? TEXT("1P") : TEXT("3P"), *GetNameSafe(Weapon))
		return false;
	}
	
	UE_LOG(LogAS, Verbose, TEXT("FireCue on %s: weapon=%s firstPerson=%d mesh=%s sound=%s"),
	*GetNameSafe(MyTarget), *GetNameSafe(Weapon), bFirstPerson, *GetNameSafe(WeaponMesh), *GetNameSafe(FireSound));
	
	CSV_CUSTOM_STAT(ArenaShooter, FireCuesPlayed, 1, ECsvCustomStatOp::Accumulate);

	if (BoundMesh.Get() != WeaponMesh)
	{
		BindToMesh(WeaponMesh, bFirstPerson);	
	}

	WakeFX();
	LastFireTime = GetWorld()->GetTimeSeconds();

	ImpactPositions.Reset();
	ImpactHits.Reset();
	if (const FGameplayEffectContext* RawContext = Parameters.EffectContext.Get())
	{
		if (RawContext->GetScriptStruct()->IsChildOf(FASGameplayEffectContext::StaticStruct()))
		{
			const FGameplayAbilityTargetDataHandle& TargetData = static_cast<const FASGameplayEffectContext*>(RawContext)->TargetData;
			for (int32 i = 0; i < TargetData.Num(); ++i)
			{
				const FGameplayAbilityTargetData* Data = TargetData.Get(i);
				if (const FHitResult* Hit = Data ? Data->GetHitResult() : nullptr)
				{
					ImpactPositions.Add(Hit->bBlockingHit ? Hit->ImpactPoint : Hit->TraceEnd);

					if (Hit->bBlockingHit)
					{
						CSV_CUSTOM_STAT(ArenaShooter, Impacts, 1, ECsvCustomStatOp::Accumulate);
						ImpactHits.Add(*Hit);
					}
				}
			}
			if (ImpactHits.Num() > 0)
			{
				UImpactStatics::SpawnImpactFX(this, ImpactChannel, ImpactSystems, ImpactHits, WeaponMesh->GetSocketLocation(MuzzleSocketName));
			}
		}
	}

	if (EnsureAwake(TracerComp, bTracerTrigger))
	{
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(TracerComp, ImpactPositionsName, ImpactPositions);
		Flip(TracerComp, bTracerTrigger);
	}

	if (EnsureAwake(MuzzleFlashComp, bMuzzleFlashTrigger))
	{
		MuzzleFlashComp->SetVariableVec3(MuzzleDirectionName, WeaponMesh->GetSocketTransform(MuzzleSocketName).GetUnitAxis(EAxis::X));
		Flip(MuzzleFlashComp, bMuzzleFlashTrigger);
	}

	if (EnsureAwake(ShellEjectComp, bShellEjectTrigger))
	{
		Flip(ShellEjectComp, bShellEjectTrigger);
	}

	if (FireSound)
	{
		UGameplayStatics::SpawnSoundAttached(FireSound, WeaponMesh, MuzzleSocketName, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	}
	
	return true;
}

UNiagaraComponent* AASGameplayCueNotify_WeaponFireActor::CreateFXComponent(UNiagaraSystem* System)
{
	UNiagaraComponent* Comp = NewObject<UNiagaraComponent>(this);
	Comp->SetAsset(System);
	Comp->SetAutoDestroy(false);
	Comp->bAutoActivate = false;
	Comp->RegisterComponent();
	AddInstanceComponent(Comp);

	return Comp;
}

void AASGameplayCueNotify_WeaponFireActor::BindToMesh(USkeletalMeshComponent* MeshComp, bool bFirstPerson)
{
	BoundMesh = MeshComp;

	if (!MuzzleFlashComp && MuzzleFlashFX)
	{
		MuzzleFlashComp = CreateFXComponent(MuzzleFlashFX);
	}

	if (!TracerComp && TracerFX)
	{
		TracerComp = CreateFXComponent(TracerFX);
	}

	if (!ShellEjectComp && ShellEjectFX)
	{
		ShellEjectComp = CreateFXComponent(ShellEjectFX);

		if (ShellEjectMesh)
		{
			ShellEjectComp->SetVariableStaticMesh(ShellMeshParamName, ShellEjectMesh);
		}
	}
	
	const EFirstPersonPrimitiveType FPType = bFirstPerson ? EFirstPersonPrimitiveType::FirstPerson : EFirstPersonPrimitiveType::None;
	const FAttachmentTransformRules Rules(FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	for (UNiagaraComponent* Comp : {MuzzleFlashComp.Get(), TracerComp.Get(), ShellEjectComp.Get()})
	{
		if (Comp)
		{
			Comp->AttachToComponent(MeshComp, Rules, MuzzleSocketName);
			Comp->SetFirstPersonPrimitiveType(FPType);
		}
	}
}

void AASGameplayCueNotify_WeaponFireActor::WakeFX()
{
	if (bWindingDown)
	{
		// A shot landed mid-wind-down. Reset every system, not just the completed ones, so all of their
		// System.CachedTrigger values agree on the default (false) - otherwise a still-active component
		// would see no edge on the flip below and silently skip this shot.
		bWindingDown = false;
		EnsureAwake(MuzzleFlashComp, bMuzzleFlashTrigger, true);
		EnsureAwake(TracerComp, bTracerTrigger, true);
		EnsureAwake(ShellEjectComp, bShellEjectTrigger, true);
	}

	if (IdleDestroyTime > 0.f && !IdleTimerHandle.IsValid())
	{
		GetWorldTimerManager().SetTimer(IdleTimerHandle, this, &AASGameplayCueNotify_WeaponFireActor::TickIdle, FMath::Max(IdleCheckInterval, 0.1f), true);
	}
}

void AASGameplayCueNotify_WeaponFireActor::ReleaseFX()
{
	if (UWorld* World = GetWorld())
	{
		GetWorldTimerManager().ClearTimer(IdleTimerHandle);	
	}
	IdleTimerHandle.Invalidate();

	for (UNiagaraComponent* Comp : {MuzzleFlashComp.Get(), TracerComp.Get(), ShellEjectComp.Get()})
	{
		if (Comp)
		{
			Comp->DestroyComponent();
		}
	}

	MuzzleFlashComp = nullptr;
	TracerComp = nullptr;
	ShellEjectComp = nullptr;
	BoundMesh = nullptr;
	bWindingDown = false;
	LastFireTime = 0.f;
	bMuzzleFlashTrigger = false;
	bTracerTrigger = false;
	bShellEjectTrigger = false;
}

bool AASGameplayCueNotify_WeaponFireActor::EnsureAwake(UNiagaraComponent* Comp, bool& State, bool bForceRestart)
{
	if (!Comp)
	{
		return false;
	}

	if (bForceRestart || !Comp->IsActive())
	{
		Comp->Activate(true);
		State = false;
	}

	return true;
}

void AASGameplayCueNotify_WeaponFireActor::Flip(UNiagaraComponent* Comp, bool& State)
{
	State = !State;
	Comp->SetVariableBool(TriggerParamName, State);
}

bool AASGameplayCueNotify_WeaponFireActor::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	ReleaseFX();
	return Super::OnRemove_Implementation(MyTarget, Parameters);
}

bool AASGameplayCueNotify_WeaponFireActor::Recycle()
{
	ReleaseFX();
	return Super::Recycle();
}


void AASGameplayCueNotify_WeaponFireActor::TickIdle()
{
	const UWorld* World = GetWorld();
	if (!World || World->TimeSince(LastFireTime) < IdleDestroyTime)
	{
		return;
	}

	if (!bWindingDown)
	{
		// Soft deactivate: spawning stops, but shells and tracers already in flight play out.
		bWindingDown = true;

		for (UNiagaraComponent* Comp : {MuzzleFlashComp.Get(), TracerComp.Get(), ShellEjectComp.Get()})
		{
			if (Comp)
			{
				Comp->Deactivate();
			}
		}
		return;
	}

	if (!AreAllSystemsInactive())
	{
		return;
	}

	// hand this instance back to the GameplayCueManager's recycle pool
	K2_EndGameplayCue();
}

bool AASGameplayCueNotify_WeaponFireActor::AreAllSystemsInactive() const
{
	for (UNiagaraComponent* Comp : {MuzzleFlashComp.Get(), TracerComp.Get(), ShellEjectComp.Get()})
	{
		if (Comp && Comp->IsActive())
		{
			return false;
		}
	}
	
	return true;
}

