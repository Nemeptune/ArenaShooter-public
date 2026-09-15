// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/PimplPtr.h"
#include "ASAvatarSubsystem.generated.h"

class UTexture2D;
struct FUniqueNetIdRepl;
struct FASSteamAvatarCallbacks;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAvatarReady, uint64 , UTexture2D* );

UCLASS()
class ARENASHOOTER_API UASAvatarSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UASAvatarSubsystem();
	virtual ~UASAvatarSubsystem() override;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

	/** 0 when the id is missing or is not a Steam id. */
	static uint64 ToPlatformUserId(const FUniqueNetIdRepl& NetId);

	/** The built texture, or null while it is still downloading. */
	UTexture2D* GetAvatar(uint64 PlatformUserId) const;

	/** Builds the texture now if Steam already has the image, otherwise asks for a download. */
	void RequestAvatar(uint64 PlatformUserId);

	/** Game thread only. Fires once per user, and again if they change their picture. */
	FOnAvatarReady OnAvatarReady;
	
private:
	friend struct FASSteamAvatarCallbacks;

	struct FPendingAvatar
	{
		double FirstRequestTime = 0.0;
		double LastInfoRequestTime = 0.0;
		int32 LastImageHandle = 0;
	};
	
	bool EnsureSteam();
	bool TryBuildAvatar(uint64 PlatformUserId);
	void RequestSteamUserInfo(uint64 PlatformUserId, FPendingAvatar& Entry, double Now);
	
	void HandleSteamUserChanged(uint64 PlatformUserId, bool bAvatarChanged);
	bool TickPending(float DeltaTime);

	UPROPERTY(Transient)
	TMap<uint64, TObjectPtr<UTexture2D>> Cache;
	
	TMap<uint64, FPendingAvatar> Pending;
	FTSTicker::FDelegateHandle PendingTicker;

	TPimplPtr<FASSteamAvatarCallbacks> SteamCallbacks;
};
