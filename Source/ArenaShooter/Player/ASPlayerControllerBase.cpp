// Fill out your copyright notice in the Description page of Project Settings.


#include "ASPlayerControllerBase.h"

#include "ASLocalPlayer.h"

void AASPlayerControllerBase::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	
	if (UASLocalPlayer* LP = Cast<UASLocalPlayer>(Player))
	{
		LP->OnPlayerControllerSet.Broadcast(LP, this);
		
		if (PlayerState)
		{
			LP->OnPlayerStateSet.Broadcast(LP, PlayerState);
		}
	}
}

void AASPlayerControllerBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	if (PlayerState)
	{
		if (UASLocalPlayer* LP = Cast<UASLocalPlayer>(Player))
		{
			LP->OnPlayerStateSet.Broadcast(LP, PlayerState);
		}
	}
}

bool AASPlayerControllerBase::ShouldShowLoadingScreen(FString& OutReason) const
{
	// The HUD is built synchronously when the PlayerState arrives, so this also means the HUD is up.
	if (!PlayerState)
	{
		OutReason = FString("Waiting for PlayerState");
		return true;
	}
	return false;
}
