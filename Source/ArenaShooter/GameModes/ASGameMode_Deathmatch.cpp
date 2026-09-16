// Fill out your copyright notice in the Description page of Project Settings.

#include "ASGameMode_Deathmatch.h"

#include "Character/ASCharacter.h"
#include "System/ASGameplayTags.h"
#include "ASGameState.h"
#include "System/ASLogChannels.h"
#include "Player/ASPlayerController.h"
#include "Player/ASPlayerState.h"
#include "AbilitySystem/ASAbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Pickups/ASPickup.h"

AASGameMode_Deathmatch::AASGameMode_Deathmatch()
{
	PlayerStateClass = AASPlayerState::StaticClass();
	GameStateClass = AASGameState::StaticClass();
	bDelayedStart = true;
}

void AASGameMode_Deathmatch::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	
	GetWorldTimerManager().SetTimer(
	TimerHandle_DefaultTimer,
	this,
	&AASGameMode_Deathmatch::DefaultTimer,
	1.f,
	true
	);
}

void AASGameMode_Deathmatch::DefaultTimer()
{
	AASGameState* GS = GetGameState<AASGameState>();
	if (!GS || GS->bTimerPaused) return;

	GS->RemainingTime--;

	if (GS->RemainingTime <= 0)
	{
		if (GetMatchState() == MatchState::InProgress)
		{
			EndMatch();
		}
		else if (GetMatchState() == MatchState::WaitingToStart)
		{
			StartMatch();
		}
	}
}

void AASGameMode_Deathmatch::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();
	if (AASGameState* GS = GetGameState<AASGameState>())
	{
		GS->RemainingTime = WarmupTime;
	}
}

void AASGameMode_Deathmatch::HandleMatchHasStarted()
{
	ResetForMatchStart();
	Super::HandleMatchHasStarted();

	if (AASGameState* GS = GetGameState<AASGameState>())
	{
		GS->RemainingTime = MatchTime;
	}
	
}

void AASGameMode_Deathmatch::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	AASGameState* GS = GetGameState<AASGameState>();
	if (!GS)
	{
		return;
	}

	GS->bTimerPaused = true;

	for (TPair<AController*, FTimerHandle>& Pair : RespawnTimers)
	{
		GetWorldTimerManager().ClearTimer(Pair.Value);
	}
	RespawnTimers.Empty();

	APlayerState* Winner = nullptr;
	int32 BestKills = -1;
	bool bTie = false;
	for (APlayerState* PS : GS->PlayerArray)
	{
		AASPlayerState* ASPS = Cast<AASPlayerState>(PS);
		if (!ASPS)
		{
			continue;
		}

		const int32 Kills = ASPS->GetKills();
		if (Kills > BestKills)
		{
			BestKills = Kills;
			Winner = ASPS;
			bTie = false;
		}
		else if (Kills == BestKills)
		{
			bTie = true;
		}
	}
	GS->SetMatchResult(bTie ? nullptr : Winner);
}

void AASGameMode_Deathmatch::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (!HasMatchStarted())
	{
		AGameModeBase::HandleStartingNewPlayer_Implementation(NewPlayer);
		return;
	}
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

bool AASGameMode_Deathmatch::PlayerCanRestart_Implementation(APlayerController* Player)
{
	if (!HasMatchStarted())
	{
		return AGameModeBase::PlayerCanRestart_Implementation(Player);
	}
	return Super::PlayerCanRestart_Implementation(Player);
}

void AASGameMode_Deathmatch::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (GetMatchState() == MatchState::WaitingToStart && NumPlayers == 2)
	{
		AASGameState* GS = GetGameState<AASGameState>();
		if (GS)
		{
			GS->RemainingTime = ShortWarmupTime;
			GS->MulticastOnPlayerJoined();
		}
	}
}

void AASGameMode_Deathmatch::OnPlayerKilled(AActor* Killer, AActor* Victim)
{
	if (!HasAuthority()) return;

	AASCharacter* VictimChar = Cast<AASCharacter>(Victim);
	if (!VictimChar) return;

	if (IsMatchInProgress())
	{
		// Credit the kill to the killer (guard against suicide / world damage)
		if (Killer && Killer != Victim)
		{
			if (APawn* KillerPawn = Cast<APawn>(Killer))
			{
				if (AASPlayerState* KillerPS = KillerPawn->GetPlayerState<AASPlayerState>())
				{
					KillerPS->AddKill();
				}
			}
		}
	
		if (AASPlayerState* VictimPS = VictimChar->GetPlayerState<AASPlayerState>())
		{
			VictimPS->AddDeath();
		}
	}

	AController* VictimController = VictimChar->GetController();
	if (!VictimController) return;
	
	if (FTimerHandle* Existing = RespawnTimers.Find(VictimController))
	{
		GetWorldTimerManager().ClearTimer(*Existing);
	}

	// Schedule the respawn. The handle lives in a member map so it can be cancelled (Logout / match end).
	// Both this and the controller are captured weakly so a teardown during the delay can't crash us.
	TWeakObjectPtr<AASGameMode_Deathmatch> WeakThis(this);
	TWeakObjectPtr<AController> WeakController(VictimController);
	FTimerHandle& Handle = RespawnTimers.FindOrAdd(VictimController);
	GetWorldTimerManager().SetTimer(Handle, [WeakThis, WeakController]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->RespawnPlayer(WeakController);
		}
	}, RespawnDelay, false);
}

void AASGameMode_Deathmatch::RespawnPlayer(TWeakObjectPtr<AController> ControllerPtr)
{
	AController* Controller = ControllerPtr.Get();
	if (!IsValid(Controller)) return;

	RespawnTimers.Remove(Controller);

	AActor* StartSpot = FindPlayerStart(Controller);
	if (!StartSpot) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AASCharacter* NewCharacter = GetWorld()->SpawnActor<AASCharacter>(DefaultPawnClass, StartSpot->GetActorLocation(), StartSpot->GetActorRotation(), SpawnParams);
	if (!NewCharacter) return;

	// Swap the controller from the old ragdoll body onto the fresh pawn. PossessedBy on the new
	// character re-grants abilities and resets Health/Shield to max.
	APawn* OldPawn = Controller->GetPawn();
	Controller->UnPossess();
	
	if (OldPawn && OldPawn != NewCharacter)
	{
		OldPawn->Destroy();
	}
	
	Controller->Possess(NewCharacter);

	if (AASPlayerController* PC = Cast<AASPlayerController>(Controller))
	{
		PC->ClientSetRotation(StartSpot->GetActorRotation());
	}

	// Signal the death ability to end now that the new body is live; this removes Status.Death.Dead.
	if (AASPlayerState* PS = Controller->GetPlayerState<AASPlayerState>())
	{
		if (UASAbilitySystemComponent* ASC = PS->GetASAbilityComponent())
		{
			FGameplayEventData Payload;
			Payload.EventTag = FASGameplayTags::Event_Respawn;
			ASC->HandleGameplayEvent(FASGameplayTags::Event_Respawn, &Payload);
		}
	}
}

void AASGameMode_Deathmatch::Logout(AController* Exiting)
{
	// Stop a pending respawn from firing for a player who has left the match.
	if (FTimerHandle* Handle = RespawnTimers.Find(Exiting))
	{
		GetWorldTimerManager().ClearTimer(*Handle);
		RespawnTimers.Remove(Exiting);
	}

	Super::Logout(Exiting);
}

void AASGameMode_Deathmatch::ResetForMatchStart()
{
	for (TPair<AController*, FTimerHandle>& Pair : RespawnTimers)
	{
		GetWorldTimerManager().ClearTimer(Pair.Value);
	}
	RespawnTimers.Empty();

	for (TActorIterator<AASPickup> It(GetWorld()); It; ++It)
	{
		It->ResetToActive();
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		if (AASPlayerState* PS = PC->GetPlayerState<AASPlayerState>())
		{
			PS->ResetScore();
			if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
			{
				ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(FASGameplayTags::Buff));
			}
		}
		
		if (AASCharacter* Character = PC->GetPawn<AASCharacter>())
		{
			Character->ReleaseLoadout();
		}

		RespawnPlayer(PC);
	}
}
