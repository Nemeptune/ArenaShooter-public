// Fill out your copyright notice in the Description page of Project Settings.


#include "ASAvatarSubsystem.h"

#include "System/ASLogChannels.h"
#include "Async/Async.h"
#include "Engine/Texture2D.h"
#include "GameFramework/OnlineReplStructs.h"
#include "OnlineSubsystem.h"
#include "TextureResource.h"
#include "OnlineSubsystemNames.h"

namespace ASAvatar
{
	/** GetLargeFriendAvatar only reads Steam's local cache, so polling it is cheap. */
	constexpr float PollIntervalSeconds = 0.5f;
	/** RequestUserInformation goes over the network; re-send it rarely. */
	constexpr double InfoRequestIntervalSeconds = 5.0;
	constexpr double GiveUpAfterSeconds = 30.0;
}

#if AS_WITH_STEAM_AVATARS
THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

struct FASSteamAvatarCallbacks
{
	explicit FASSteamAvatarCallbacks(UASAvatarSubsystem* InOwner) : Owner(InOwner) {}

	STEAM_CALLBACK(FASSteamAvatarCallbacks, OnAvatarImageLoaded, AvatarImageLoaded_t);
	STEAM_CALLBACK(FASSteamAvatarCallbacks, OnPersonaStateChange, PersonaStateChange_t);

	void Rebuild(uint64 PlatformUserId) const
	{
		TWeakObjectPtr<UASAvatarSubsystem> WeakOwner = Owner;
		AsyncTask(ENamedThreads::GameThread, [WeakOwner, PlatformUserId]()
		{
			if (UASAvatarSubsystem* Subsystem = WeakOwner.Get())
			{
				Subsystem->TryBuildAvatar(PlatformUserId);
			}
		});
	}

	TWeakObjectPtr<UASAvatarSubsystem> Owner;
};

void FASSteamAvatarCallbacks::OnAvatarImageLoaded(AvatarImageLoaded_t* Param)
{
	if (Param)
	{
		Rebuild(Param->m_steamID.ConvertToUint64());
	}
}

void FASSteamAvatarCallbacks::OnPersonaStateChange(PersonaStateChange_t* Param)
{
	if (Param && (Param->m_nChangeFlags & k_EPersonaChangeAvatar) != 0)
	{
		Rebuild(Param->m_ulSteamID);
	}
}
#endif

UASAvatarSubsystem::UASAvatarSubsystem() = default;
UASAvatarSubsystem::~UASAvatarSubsystem() = default;

bool UASAvatarSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return Super::ShouldCreateSubsystem(Outer) && !IsRunningDedicatedServer();
}

void UASAvatarSubsystem::Deinitialize()
{
	FTSTicker::RemoveTicker(PendingTicker);
	PendingTicker.Reset();
	SteamCallbacks.Reset();
	Pending.Empty();
	Cache.Empty();
	Super::Deinitialize();
}

bool UASAvatarSubsystem::EnsureSteam()
{
#if AS_WITH_STEAM_AVATARS
	if (SteamCallbacks.IsValid())
	{
		return true;
	}
	// Touching the subsystem forces OnlineSubsystemSteam, and SteamAPI_Init, to come up first.
	IOnlineSubsystem* Steam = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
	if (Steam == nullptr || SteamFriends() == nullptr || SteamUtils() == nullptr)
	{
		UE_LOG(LogAS, Warning, TEXT("Avatar: Steam unavailable. OSS=%d Friends=%d Utils=%d"),
			Steam != nullptr, SteamFriends() != nullptr, SteamUtils() != nullptr);
		return false;
	}
	UE_LOG(LogAS, Log, TEXT("Avatar: Steam ready, callbacks registered."));
	SteamCallbacks = MakePimpl<FASSteamAvatarCallbacks>(this);
	return true;
#else
	return false;
#endif
}

uint64 UASAvatarSubsystem::ToPlatformUserId(const FUniqueNetIdRepl& NetId)
{
	// Invalid until the server assigns it or it replicates; OnUniqueIdChanged refreshes then.
	if (!NetId.IsValid() || NetId.GetType() != STEAM_SUBSYSTEM)
	{
		return 0;
	}
	return FCString::Strtoui64(*NetId->ToString(), nullptr, 10);
}

UTexture2D* UASAvatarSubsystem::GetAvatar(uint64 PlatformUserId) const
{
	const TObjectPtr<UTexture2D>* Found = Cache.Find(PlatformUserId);
	return Found ? Found->Get() : nullptr;
}

void UASAvatarSubsystem::RequestAvatar(uint64 PlatformUserId)
{
	if (PlatformUserId == 0 || GetAvatar(PlatformUserId) || Pending.Contains(PlatformUserId) || !EnsureSteam())
	{
		return;
	}
	if (TryBuildAvatar(PlatformUserId))
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	FPendingAvatar& Entry = Pending.Add(PlatformUserId);
	Entry.FirstRequestTime = Now;
	RequestSteamUserInfo(PlatformUserId, Entry, Now);

	if (!PendingTicker.IsValid())
	{
		PendingTicker = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UASAvatarSubsystem::TickPending), ASAvatar::PollIntervalSeconds);
	}
}

void UASAvatarSubsystem::RequestSteamUserInfo(uint64 PlatformUserId, FPendingAvatar& Entry, double Now)
{
	Entry.LastInfoRequestTime = Now;
#if AS_WITH_STEAM_AVATARS
	if (ISteamFriends* Friends = SteamFriends())
	{
		// false = Steam thinks it already has this user. The poll still picks the image up when it lands.
		const bool bStarted = Friends->RequestUserInformation(CSteamID(PlatformUserId), false);
		UE_LOG(LogAS, Log, TEXT("Avatar: %llu not ready, RequestUserInformation started=%d"), PlatformUserId, bStarted);
	}
#endif
}

void UASAvatarSubsystem::HandleSteamUserChanged(uint64 PlatformUserId, bool bAvatarChanged)
{
	// Pending users retry on any update; cached users rebuild only when the picture changed.
	if (Pending.Contains(PlatformUserId) || (bAvatarChanged && GetAvatar(PlatformUserId)))
	{
		TryBuildAvatar(PlatformUserId);
	}
}

bool UASAvatarSubsystem::TickPending(float)
{
	const double Now = FPlatformTime::Seconds();

	// Copy the keys: TryBuildAvatar broadcasts, and a listener may request another id.
	TArray<uint64> Ids;
	Pending.GenerateKeyArray(Ids);

	for (const uint64 Id : Ids)
	{
		if (!Pending.Contains(Id) || TryBuildAvatar(Id))
		{
			continue;
		}
		FPendingAvatar& Entry = Pending.FindChecked(Id);
		if (Now - Entry.FirstRequestTime > ASAvatar::GiveUpAfterSeconds)
		{
			UE_LOG(LogAS, Warning, TEXT("Avatar: gave up on %llu after %.0fs, last handle %d (0 = no data, -1 = download never finished)."),
				Id, ASAvatar::GiveUpAfterSeconds, Entry.LastImageHandle);
			Pending.Remove(Id);
		}
		else if (Now - Entry.LastInfoRequestTime > ASAvatar::InfoRequestIntervalSeconds)
		{
			RequestSteamUserInfo(Id, Entry, Now);
		}
	}

	if (Pending.Num() == 0)
	{
		PendingTicker.Reset();
		return false;
	}
	return true;
}

bool UASAvatarSubsystem::TryBuildAvatar(uint64 PlatformUserId)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UASAvatarSubsystem::TryBuildAvatar);
	
#if AS_WITH_STEAM_AVATARS
	check(IsInGameThread());
	ISteamFriends* Friends = SteamFriends();
	ISteamUtils* Utils = SteamUtils();
	if (PlatformUserId == 0 || Friends == nullptr || Utils == nullptr)
	{
		return false;
	}

	// 0: Steam has no avatar data for this user (yet). -1: the image is queued for download.
	const int ImageHandle = Friends->GetLargeFriendAvatar(CSteamID(PlatformUserId));
	if (FPendingAvatar* Entry = Pending.Find(PlatformUserId))
	{
		Entry->LastImageHandle = ImageHandle;
	}
	if (ImageHandle <= 0)
	{
		return false;
	}

	uint32 Width = 0;
	uint32 Height = 0;
	if (!Utils->GetImageSize(ImageHandle, &Width, &Height) || Width == 0 || Height == 0)
	{
		return false;
	}

	TArray<uint8> Pixels;
	Pixels.SetNumUninitialized(static_cast<int32>(Width * Height * 4));
	if (!Utils->GetImageRGBA(ImageHandle, Pixels.GetData(), Pixels.Num()))
	{
		return false;
	}

	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
	if (!Texture)
	{
		return false;
	}
	Texture->SRGB = true;
	Texture->NeverStream = true;
	Texture->Filter = TF_Bilinear;

	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	void* Dest = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Dest, Pixels.GetData(), Pixels.Num());
	Mip.BulkData.Unlock();
	Texture->UpdateResource();

	Cache.Add(PlatformUserId, Texture);
	Pending.Remove(PlatformUserId);
	UE_LOG(LogAS, Log, TEXT("Avatar: built %ux%u texture for %llu."), Width, Height, PlatformUserId);

	OnAvatarReady.Broadcast(PlatformUserId, Texture);
	return true;
#else
	return false;
#endif
}