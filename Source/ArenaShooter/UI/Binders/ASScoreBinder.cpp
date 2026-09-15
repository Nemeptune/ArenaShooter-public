// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Binders/ASScoreBinder.h"

#include "GameModes/ASGameState.h"
#include "System/ASLogChannels.h"
#include "Online/ASAvatarSubsystem.h"
#include "Player/ASPlayerState.h"
#include "UI/ASUISettings.h"
#include "UI/ViewModels/ASScoreViewModel.h"

void UASScoreBinder::Init(AASPlayerState* InLocalPS, UASScoreViewModel* InViewModel)
{
	if (!InLocalPS || !InViewModel)
	{
		return;
	}
	LocalPS = InLocalPS;
	ViewModel = InViewModel;

	Activate();
}

void UASScoreBinder::SubscribeAll()
{
	if (LocalPS)
	{
		LocalPS->OnKillsChanged.AddDynamic(this, &UASScoreBinder::HandleLocalKills);
		LocalPS->OnNameChanged.AddDynamic(this, &UASScoreBinder::HandleLocalNameChanged);
		LocalPS->OnUniqueIdChanged.AddDynamic(this, &UASScoreBinder::HandleUniqueIdChanged);
	}
	if (UASAvatarSubsystem* Avatars = GetAvatarSubsystem())
	{
		BoundAvatars = Avatars;
		AvatarHandle = Avatars->OnAvatarReady.AddUObject(this, &UASScoreBinder::HandleAvatarReady);
	}
	BindWorld();
}

void UASScoreBinder::BindWorld()
{
	if (bWorldBound || !IsActive())
	{
		return;
	}

	UWorld* World = GetWorld();
	AASGameState* GS = World ? GetWorld()->GetGameState<AASGameState>() : nullptr;
	if (!GS)
	{
		if (World)
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UASScoreBinder::BindWorld));
		}
		return;
	}
	bWorldBound = true;
	BoundGS = GS;

	GS->OnPlayerAdded.AddUObject(this, &UASScoreBinder::HandlePlayerAdded);
	GS->OnPlayerRemoved.AddUObject(this, &UASScoreBinder::HandlePlayerRemoved);
	
	World->GetTimerManager().SetTimer(Timer, this, &UASScoreBinder::UpdateTimer, 1.f, true);
	
	RefreshAll();
}

void UASScoreBinder::UnSubscribeAll()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Timer);
	}
	if (LocalPS)
	{
		LocalPS->OnKillsChanged.RemoveDynamic(this, &UASScoreBinder::HandleLocalKills);
		LocalPS->OnNameChanged.RemoveDynamic(this, &UASScoreBinder::HandleLocalNameChanged);
		LocalPS->OnUniqueIdChanged.RemoveDynamic(this, &UASScoreBinder::HandleUniqueIdChanged);
	}
	if (OpponentPS)
	{
		OpponentPS->OnKillsChanged.RemoveDynamic(this, &UASScoreBinder::HandleOpponentKills);
		OpponentPS->OnNameChanged.RemoveDynamic(this, &UASScoreBinder::HandleOpponentNameChanged);
		OpponentPS->OnUniqueIdChanged.RemoveDynamic(this, &UASScoreBinder::HandleUniqueIdChanged);
	}
	if (AASGameState* GS = BoundGS.Get())
	{
		GS->OnPlayerAdded.RemoveAll(this);
		GS->OnPlayerRemoved.RemoveAll(this);
	}

	BoundGS = nullptr;
	
	if (UASAvatarSubsystem* Avatars = BoundAvatars.Get())
	{
		Avatars->OnAvatarReady.Remove(AvatarHandle);
	}
	AvatarHandle.Reset();
	BoundAvatars = nullptr;
	LocalPS = nullptr;
	OpponentPS = nullptr;
	ViewModel = nullptr;
	bWorldBound = false;
}

void UASScoreBinder::RefreshAll()
{
	if (!ViewModel)
	{
		return;
	}
	
	ViewModel->SetLocalName(LocalPS ? FText::FromString(LocalPS->GetPlayerName()) : FText::GetEmpty());
	ViewModel->SetLocalKills(LocalPS? LocalPS->GetKills() : 0);
	
	if (AASGameState* GS = BoundGS.Get())
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			HandlePlayerAdded(PS);
		}
	}
	RefreshAvatars();
	UpdateTimer();
}

UASAvatarSubsystem* UASScoreBinder::GetAvatarSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UASAvatarSubsystem>() : nullptr;
}

void UASScoreBinder::RefreshAvatars()
{
	UASAvatarSubsystem* Avatars = BoundAvatars.Get();
	if (!Avatars || !ViewModel)
	{
		return;
	}

	const uint64 LocalId = LocalPS ? UASAvatarSubsystem::ToPlatformUserId(LocalPS->GetUniqueId()) : 0;
	const uint64 OpponentId = OpponentPS ? UASAvatarSubsystem::ToPlatformUserId(OpponentPS->GetUniqueId()) : 0;
	
	UE_LOG(LogAS, Log, TEXT("Avatar: refresh local=%llu opponent=%llu"), LocalId, OpponentId);

	ViewModel->SetLocalAvatar(Avatars->GetAvatar(LocalId));
	ViewModel->SetOpponentAvatar(Avatars->GetAvatar(OpponentId));
	
	Avatars->RequestAvatar(LocalId);
	Avatars->RequestAvatar(OpponentId);
}

void UASScoreBinder::HandleAvatarReady(uint64 PlatformUserId, UTexture2D* Avatar)
{
	if (!ViewModel)
	{
		return;
	}
	if (LocalPS && UASAvatarSubsystem::ToPlatformUserId(LocalPS->GetUniqueId()) == PlatformUserId)
	{
		ViewModel->SetLocalAvatar(Avatar);
	}
	if (OpponentPS && UASAvatarSubsystem::ToPlatformUserId(OpponentPS->GetUniqueId()) == PlatformUserId)
	{
		ViewModel->SetOpponentAvatar(Avatar);
	}
}

void UASScoreBinder::HandleUniqueIdChanged()
{
	RefreshAvatars();
}

void UASScoreBinder::HandlePlayerAdded(APlayerState* PlayerState)
{
	if (!PlayerState || PlayerState == LocalPS || OpponentPS) return;
	AASPlayerState* OppPS = Cast<AASPlayerState>(PlayerState);
	if (!OppPS) return;

	OpponentPS = OppPS;
	if (ViewModel)
	{
		ViewModel->SetOpponentName(FText::FromString(OppPS->GetPlayerName()));
		ViewModel->SetOpponentKills(OppPS->GetKills());
	}
	OppPS->OnKillsChanged.AddDynamic(this, &UASScoreBinder::HandleOpponentKills);
	OppPS->OnNameChanged.AddDynamic(this, &UASScoreBinder::HandleOpponentNameChanged);
	OppPS->OnUniqueIdChanged.AddDynamic(this, &UASScoreBinder::HandleUniqueIdChanged);

	RefreshAvatars();
}

void UASScoreBinder::HandlePlayerRemoved(APlayerState* PlayerState)
{
	if (PlayerState != OpponentPS || !OpponentPS) return;
	OpponentPS->OnKillsChanged.RemoveDynamic(this, &UASScoreBinder::HandleOpponentKills);
	OpponentPS->OnNameChanged.RemoveDynamic(this, &UASScoreBinder::HandleOpponentNameChanged);
	OpponentPS->OnUniqueIdChanged.RemoveDynamic(this, &UASScoreBinder::HandleUniqueIdChanged);
	OpponentPS = nullptr;
	if (ViewModel)
	{
		ViewModel->SetOpponentName(FText::GetEmpty());
		ViewModel->SetOpponentKills(0); // TODO change this when reconnect support added
		ViewModel->SetOpponentAvatar(nullptr);
	}
}

void UASScoreBinder::UpdateTimer()
{
	if (!ViewModel)
	{
		return;	
	}
	AASGameState* GS = GetWorld() ? GetWorld()->GetGameState<AASGameState>() : nullptr;
	const int32 RemainingTime = GS ? FMath::Max(0, GS->RemainingTime) : 0;
	ViewModel->SetMatchTimeText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), RemainingTime / 60, RemainingTime % 60)));
}

void UASScoreBinder::HandleLocalNameChanged()
{
	if (ViewModel && LocalPS)
	{
		ViewModel->SetLocalName(FText::FromString(LocalPS->GetPlayerName()));
	}
}

void UASScoreBinder::HandleOpponentNameChanged()
{
	if (ViewModel && OpponentPS)
	{
		ViewModel->SetOpponentName(FText::FromString(OpponentPS->GetPlayerName()));
	}
}

void UASScoreBinder::HandleLocalKills(int32 NewKills)
{
	if (ViewModel)
	{
		ViewModel->SetLocalKills(NewKills);
	}
}

void UASScoreBinder::HandleOpponentKills(int32 NewKills)
{
	if (ViewModel)
	{
		ViewModel->SetOpponentKills(NewKills);
	}
}
