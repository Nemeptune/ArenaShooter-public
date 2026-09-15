// Fill out your copyright notice in the Description page of Project Settings.


#include "ASAT_SpawnPredictedProjectile.h"

#include "AbilitySystemComponent.h"
#include "ASLogChannels.h"
#include "Player/ASPlayerController.h"
#include "AbilitySystem/ASAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Weapon/ASProjectile.h"

UASAT_SpawnPredictedProjectile* UASAT_SpawnPredictedProjectile::SpawnPredictedProjectile(UGameplayAbility* OwningAbility, TSubclassOf<AASProjectile> ProjectileClass, FVector SpawnLocation, FRotator SpawnRotation)
{
	if (!ensureAlwaysMsgf(OwningAbility->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted, TEXT("SpawnPredictedProjectile ability task activated in ability (%s), which does not have a net execution policy of Local Predicted. SpawnPredictedProjectile should only be used in predicted abilities. Use AbilityTask_SpawnActor otherwise."), *GetNameSafe(OwningAbility)))
	{
		return nullptr;
	}

	if (!ensureAlwaysMsgf(IsValid(ProjectileClass), TEXT("SpawnPredictedProjectile ability task activated in ability (%s) without a valid projectile class set."), *GetNameSafe(OwningAbility)))
	{
		return nullptr;
	}

	UASAT_SpawnPredictedProjectile* Task = NewAbilityTask<UASAT_SpawnPredictedProjectile>(OwningAbility);
	Task->ProjectileClass = ProjectileClass;
	Task->SpawnLocation = SpawnLocation;
	Task->SpawnRotation = SpawnRotation;
	return Task;
}

void UASAT_SpawnPredictedProjectile::Activate()
{
	Super::Activate();

	if (IsPredictingClient())
	{
		GetActivationPredictionKey().NewRejectedDelegate().BindUObject(this, &UASAT_SpawnPredictedProjectile::OnTaskRejected);
	}

	if (Ability && Ability->GetCurrentActorInfo())
	{
		if (AASPlayerController* PC = Ability->GetCurrentActorInfo()->PlayerController.IsValid() ? Cast<AASPlayerController>(Ability->GetCurrentActorInfo()->PlayerController.Get()) : nullptr)
		{
			const float ForwardPredictionTime = PC->GetForwardPredictionTime();
			const bool bIsNetAuthority = Ability->GetCurrentActorInfo()->IsNetAuthority();
			const bool bShouldUseServerInfo = IsLocallyControlled();

			if (bIsNetAuthority && !bShouldUseServerInfo)
			{
				const FGameplayAbilitySpecHandle& SpecHandle = GetAbilitySpecHandle();
				const FPredictionKey& ActivationPredictionKey = GetActivationPredictionKey();

				AbilitySystemComponent->AbilityTargetDataSetDelegate(SpecHandle, ActivationPredictionKey).AddUObject(this, &UASAT_SpawnPredictedProjectile::OnSpawnDataReplicated);
				AbilitySystemComponent->AbilityTargetDataCancelledDelegate(SpecHandle, ActivationPredictionKey).AddUObject(this, &UASAT_SpawnPredictedProjectile::OnSpawnDataCancelled);

				// Check if client already sent data
				AbilitySystemComponent->CallReplicatedTargetDataDelegatesIfSet(SpecHandle, ActivationPredictionKey);

				// Kill the ability if we never receive the data
				SetWaitingOnRemotePlayerData();

				return;
			}

			if (!bIsNetAuthority)
			{
				/* On clients, if our ping is too high to forward-predict, delay spawning the projectile so we don't
				 * forward-predict further than MaxPredictionPing. */
				float SleepTime = PC->GetProjectileSleepTime();
				if (SleepTime > 0.0f)
				{
					if (!GetWorld()->GetTimerManager().IsTimerActive(SpawnDelayedFakeProjHandle))
					{
						DelayedProjectileInfo.ProjectileClass = ProjectileClass;
						DelayedProjectileInfo.SpawnLocation = SpawnLocation;
						DelayedProjectileInfo.SpawnRotation = SpawnRotation;
						DelayedProjectileInfo.ArenaPC = PC;
						DelayedProjectileInfo.ProjectileId = PC->GenerateNewFakeProjectileID();
						GetWorld()->GetTimerManager().SetTimer(SpawnDelayedFakeProjHandle, this, &UASAT_SpawnPredictedProjectile::SpawnDelayedFakeProjectile, SleepTime, false);

						PROJECTILE_LOG(Verbose, TEXT("(%i:%i.%i) (ID: %i): Spawning fake projectile delayed. Ping (%fms) exceeds maximum prediction time. Sleeping for (%fms) to forward-predict with maximum time (%fms) and latency reduction (%fms)."),
							FDateTime::UtcNow().GetMinute(), FDateTime::UtcNow().GetSecond(), FDateTime::UtcNow().GetMillisecond(),
							DelayedProjectileInfo.ProjectileId, PC->PlayerState->ExactPing, SleepTime * 1000.0f, ForwardPredictionTime * 1000.0f, PC->PredictionLatencyReduction);
					}
					
					return;
				}
				
				// If our ping is low enough to forward-predict (or we're on LAN), immediately spawn and initialize the fake projectile.
				const uint32 FakeProjectileId = PC->GenerateNewFakeProjectileID();
				if (AASProjectile* NewProjectile = GetWorld()->SpawnActor<AASProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, GenerateSpawnParamsForFake(FakeProjectileId)))
				{
					PROJECTILE_LOG(Verbose, TEXT("(%i:%i.%i) (ID: %i): Successfully spawned fake projectile (%s) on time. Attempting to forward-predict (%fms) with ping (%fms). Client bias: (%i%%)."),
						FDateTime::UtcNow().GetMinute(), FDateTime::UtcNow().GetSecond(), FDateTime::UtcNow().GetMillisecond(), FakeProjectileId, *GetNameSafe(NewProjectile), ForwardPredictionTime * 1000.0f,
						PC->PlayerState->ExactPing, (uint32)(PC->ClientBiasPct * 100.0f));
					
					SendSpawnDataToServer(SpawnLocation, SpawnRotation, FakeProjectileId);
					
					// Cache the projectile in case the server rejects this task, and we have to destroy it.
					SpawnedFakeProj = NewProjectile;
					
					if (ShouldBroadcastAbilityTaskDelegates())
					{
						Success.Broadcast(NewProjectile);
					}
					// We don't end the task here because we need to keep listening for possible rejection.
					return;
				}
			}
			else if (bIsNetAuthority && bShouldUseServerInfo)
			{
				if (AASProjectile* NewProjectile = GetWorld()->SpawnActor<AASProjectile>(ProjectileClass, SpawnLocation, SpawnRotation,GenerateSpawnParams()))
				{
					PROJECTILE_LOG(Verbose, TEXT("(%i:%i.%i) (ID: N/A): Successfully spawned authoritative projectile (%s) on time for local server. No prediction performed."),
						FDateTime::UtcNow().GetMinute(), FDateTime::UtcNow().GetSecond(), FDateTime::UtcNow().GetMillisecond(), *GetNameSafe(NewProjectile));

					if (ShouldBroadcastAbilityTaskDelegates())
					{
						Success.Broadcast(NewProjectile);
					}

					EndTask();

					return;
				}
			}
		}
	}

	CancelServerSpawn();

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FailedToSpawn.Broadcast(nullptr);
	}

	EndTask();
}

void UASAT_SpawnPredictedProjectile::SpawnDelayedFakeProjectile()
{
	if (Ability && Ability->GetCurrentActorInfo() && DelayedProjectileInfo.ArenaPC.IsValid())
	{
		if (AASProjectile* NewProjectile = GetWorld()->SpawnActor<AASProjectile>(DelayedProjectileInfo.ProjectileClass, DelayedProjectileInfo.SpawnLocation, DelayedProjectileInfo.SpawnRotation, GenerateSpawnParamsForFake(DelayedProjectileInfo.ProjectileId)))
		{
			PROJECTILE_LOG(Verbose, TEXT("(%i:%i.%i) (ID: %i): Successfully spawned fake projectile (%s) delayed. Attempting to forward-predict (%fms) with ping (%fms)."),
				FDateTime::UtcNow().GetMinute(), FDateTime::UtcNow().GetSecond(), FDateTime::UtcNow().GetMillisecond(),
				DelayedProjectileInfo.ProjectileId, *GetNameSafe(NewProjectile), DelayedProjectileInfo.ArenaPC->GetForwardPredictionTime() * 1000.0f, DelayedProjectileInfo.ArenaPC->PlayerState->ExactPing);

			SendSpawnDataToServer(DelayedProjectileInfo.SpawnLocation, DelayedProjectileInfo.SpawnRotation, DelayedProjectileInfo.ProjectileId);
			
			SpawnedFakeProj = NewProjectile;

			if (ShouldBroadcastAbilityTaskDelegates())
			{
				Success.Broadcast(NewProjectile);
			}
			
			// We don't end the task here because we need to keep listening for possible rejection.

			return;
		}
	}

	CancelServerSpawn();

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FailedToSpawn.Broadcast(nullptr);
	}

	EndTask();
}

FActorSpawnParameters UASAT_SpawnPredictedProjectile::GenerateSpawnParams() const
{
	FActorSpawnParameters Params;
	Params.Instigator = Ability->GetCurrentActorInfo()->AvatarActor.IsValid() ? Cast<APawn>(Ability->GetCurrentActorInfo()->AvatarActor.Get()) : nullptr;
	Params.Owner = AbilitySystemComponent->GetOwnerActor();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return Params;
}

FActorSpawnParameters UASAT_SpawnPredictedProjectile::GenerateSpawnParamsForFake(const uint32 ProjectileId) const
{
	FActorSpawnParameters Params = GenerateSpawnParams();
	
	AASPlayerController* PlayerController = Cast<AASPlayerController>(Ability->GetCurrentActorInfo()->PlayerController);	

	
	Params.CustomPreSpawnInitialization = [ProjectileId, PlayerController](AActor* Actor)
	{
		if (AASProjectile* Projectile = Cast<AASProjectile>(Actor))
		{
			Projectile->InitProjectileId(ProjectileId);
			Projectile->InitFakeProjectileId(PlayerController, ProjectileId);
		}
	};
	return Params;
}

FActorSpawnParameters UASAT_SpawnPredictedProjectile::GenerateSpawnParamsForAuth(const uint32 ProjectileId) const
{
	FActorSpawnParameters Params = GenerateSpawnParams();
	Params.CustomPreSpawnInitialization = [ProjectileId](AActor* Actor)
	{
		if (AASProjectile* Projectile = Cast<AASProjectile>(Actor))
		{
			Projectile->InitProjectileId(ProjectileId);
		}
	};
	return Params;
}

void UASAT_SpawnPredictedProjectile::SendSpawnDataToServer(const FVector& InLocation, const FRotator& InRotation, uint32 InProjectileId)
{
	const bool bGenerateNewKey = !AbilitySystemComponent->ScopedPredictionKey.IsValidForMorePrediction();
	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent.Get(), bGenerateNewKey);
	FGameplayAbilityTargetDataHandle Handle = FGameplayAbilityTargetData_ProjectileSpawnInfo::MakeProjectileSpawnInfoTargetData(InLocation, InRotation, InProjectileId);
	AbilitySystemComponent->CallServerSetReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey(),Handle, FGameplayTag(), AbilitySystemComponent->ScopedPredictionKey);
}

void UASAT_SpawnPredictedProjectile::OnSpawnDataReplicated(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag Activation)
{
	// copy target data before we consume it
	const FGameplayAbilityTargetData* TargetData = Data.Get(0);

	// consume the client's data. Ensures each server task spawns only one projectile for each client task
	if (!Cast<UASAbilitySystemComponent>(AbilitySystemComponent)->TryConsumeClientReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey()))
	{
		return;
	}

	if (TargetData)
	{
		if (const FGameplayAbilityTargetData_ProjectileSpawnInfo* SpawnInfo = static_cast<const FGameplayAbilityTargetData_ProjectileSpawnInfo*>(TargetData))
		{
			AASPlayerController* PC = Ability->GetCurrentActorInfo()->PlayerController.IsValid() ? Cast<AASPlayerController>(Ability->GetCurrentActorInfo()->PlayerController.Get()) : nullptr;
			const float ForwardPredictionTime =	PC->GetForwardPredictionTime();

			if (AASProjectile* NewProjectile = GetWorld()->SpawnActor<AASProjectile>(ProjectileClass, SpawnInfo->SpawnLocation, SpawnInfo->SpawnRotation, GenerateSpawnParamsForAuth(SpawnInfo->ProjectileId)))
			{
				PROJECTILE_LOG(Verbose, TEXT("(%i:%i.%i) (ID: %i): Successfully spawned authoritative projectile (%s). Forwarded (%fms) for perceived ping (%fms). Latency reduction: (%fms) Client bias: (%i%%)"),
					FDateTime::UtcNow().GetMinute(), FDateTime::UtcNow().GetSecond(), FDateTime::UtcNow().GetMillisecond(), SpawnInfo->ProjectileId, *GetNameSafe(NewProjectile), ForwardPredictionTime * 1000.0f,
					PC->PlayerState->ExactPing, PC->PredictionLatencyReduction, (uint32)(PC->ClientBiasPct * 100.0f));
				
				if (NewProjectile->ProjectileMovement)
				{
					if (NewProjectile->PrimaryActorTick.IsTickFunctionEnabled())
					{
						NewProjectile->TickActor(ForwardPredictionTime * NewProjectile->CustomTimeDilation, LEVELTICK_All, NewProjectile->PrimaryActorTick);
					}

					NewProjectile->ProjectileMovement->TickComponent(ForwardPredictionTime * NewProjectile->CustomTimeDilation, LEVELTICK_All, nullptr);

					if (NewProjectile->GetLifeSpan() > 0.f)
					{
						/* subtract the forward prediction time from its lifespan.
						 * Clamp at 0.2 so we have enough time to replicate. */
						NewProjectile->SetLifeSpan(FMath::Max(0.2f, NewProjectile->GetLifeSpan() - ForwardPredictionTime));
					}
				}

				if (ShouldBroadcastAbilityTaskDelegates())
				{
					Success.Broadcast(NewProjectile);
				}

				EndTask();
				return;
			}
		}
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FailedToSpawn.Broadcast(nullptr);
	}

	EndTask();
}

void UASAT_SpawnPredictedProjectile::OnSpawnDataCancelled()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FailedToSpawn.Broadcast(nullptr);
	}

	EndTask();
}

void UASAT_SpawnPredictedProjectile::CancelServerSpawn()
{
	const bool bGenerateNewKey = !AbilitySystemComponent->ScopedPredictionKey.IsValidForMorePrediction();
	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent.Get(), bGenerateNewKey);
	AbilitySystemComponent->ServerSetReplicatedTargetDataCancelled(GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
}

void UASAT_SpawnPredictedProjectile::OnTaskRejected()
{
	PROJECTILE_LOG(Verbose, TEXT("SpawnPredictedProjectile task in ability (%s) rejected. Destroying fake projectile (%s)..."),
	*GetNameSafe(Ability), *GetNameSafe(SpawnedFakeProj.Get()));

	AASPlayerController* PC = (Ability && Ability->GetCurrentActorInfo()) ? Cast<AASPlayerController>(Ability->GetCurrentActorInfo()->PlayerController.Get()) : nullptr;
	// if we spawned fake projectile on the client - destroy it and clear it from PC's list of unlinked projectiles
	if (SpawnedFakeProj.IsValid())
	{
		if (PC)
		{
			if (const uint32* Key = PC->FakeProjectiles.FindKey(SpawnedFakeProj.Get()))
			{
				PROJECTILE_LOG(Error, TEXT("Removed %i"), *Key);
				PC->FakeProjectiles.Remove(*Key);
			}
		}

		SpawnedFakeProj->Destroy();
	}

	// If we didn't spawn the fake projectile yet (because we're waiting for a delayed spawn), cancel it.
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}
