// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ASCharacterMovementComponent.generated.h"

struct FASMoveState
{
	float DashCooldownRemaining = 0.f;

	// Time walking since the last landing. Inside the bunny-hop window a landing doesn't end the chain.
	float TimeOnGround = 0.f;

	// Speed earned by the running bunny-hop chain. Zero - no chain.
	float BunnyHopSpeed = 0.f;
	
	// Need cooldown for gain, so player would not hop multiple times at once at slopes.
	float BunnyHopGainCooldown = 0.f;

	void Serialize(FArchive& Ar)
	{
		Ar << DashCooldownRemaining;
		Ar << TimeOnGround;
		Ar << BunnyHopSpeed;
		Ar << BunnyHopGainCooldown;
	}
};

struct FASCharacterMoveResponseDataContainer : public FCharacterMoveResponseDataContainer
{
	using Super = FCharacterMoveResponseDataContainer;

	virtual void ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment) override;
	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap) override;
	
	FASMoveState MoveState;
};

/** Move data the client sends: the engine's, plus the moment the client was seeing. */
struct FASCharacterNetworkMoveData : public FCharacterNetworkMoveData
{
	using Super = FCharacterNetworkMoveData;

	// Server time of the world the client showed when it made this move. Zero before its first snapshot.
	double ViewServerTime = 0.;

	virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;
};

struct FASCharacterNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{
	FASCharacterNetworkMoveDataContainer()
	{
		NewMoveData = &ASMoveData[0];
		PendingMoveData = &ASMoveData[1];
		OldMoveData = &ASMoveData[2];
	}

private:
	FASCharacterNetworkMoveData ASMoveData[3];
};

class FSavedMove_ASCharacter : public FSavedMove_Character
{
public:
	using Super = FSavedMove_Character;
	
	virtual void Clear() override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void SetInitialPosition(ACharacter* C) override;
	virtual uint8 GetCompressedFlags() const override;
	virtual void CombineWith(const FSavedMove_Character* OldMove, ACharacter* InCharacter, APlayerController* PC, const FVector& OldStartLocation) override;
	double GetViewServerTime() const;
private:
	static class UASCharacterMovementComponent* GetMovement(const ACharacter* Character);
	
	bool bSavedWantsToDash = false;
	FASMoveState StartMoveState;
	double SavedViewServerTime = 0.f;
};

class FNetworkPredictionData_Client_ASCharacter : public FNetworkPredictionData_Client_Character
{
public:
	explicit FNetworkPredictionData_Client_ASCharacter(const UCharacterMovementComponent& ClientMovement)
	: FNetworkPredictionData_Client_Character(ClientMovement){}
	
	virtual FSavedMovePtr AllocateNewMove() override;
};

UCLASS()
class ARENASHOOTER_API UASCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	friend class FSavedMove_ASCharacter;
public:
	
	UASCharacterMovementComponent();

	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Knockback")
	void AddDampedImpulse(FVector Impulse, bool bSelfInflicted);

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Knockback")
	float KnockbackDampingSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Knockback")
	float MaxKnockbackHorizontalVelocity = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Knockback")
	float MaxUndampedImpulseZ = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Knockback")
	float MaxAdditiveKnockbackZ = 750.f;
	
	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Dash")
	void RequestDash();

	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Dash")
	float GetDashCooldownRemaining() const;
	
	// Speed along the dash direction. A player already moving faster that way keeps their speed.
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Dash", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float DashSpeed = 1400.f;

	// Minimum upward speed after a dash. Keep it above zero: a ground dash has to leave the floor or ground friction eats it.
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Dash", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float DashLiftSpeed = 300.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Dash", meta = (ClampMin = "0"))
	float DashCooldown = 1.5f;
	
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|BunnyHop", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float BunnyHopSpeedGain = 100.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|BunnyHop", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float BunnyHopMaxSpeed = 1500.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|BunnyHop", meta = (ClampMin = "0"))
	float BunnyHopWindow = 0.15f;
	
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|BunnyHop", meta = (ClampMin = "0", ClampMax = "90"))
	float BunnyHopWallBreakAngle = 45.f;
	
	// Minimum seconds between hops that add speed. Far below a real hop's airtime, far above one move.
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|BunnyHop", meta = (ClampMin = "0"))
	float BunnyHopMinGainInterval = 0.25f;
	
	const FASMoveState& GetMoveState() const;
	
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;
	
protected:
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f, const FVector& MoveDelta = FVector::ZeroVector) override;
	
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual bool ClientUpdatePositionAfterServerUpdate() override;
	virtual void ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration) override;
	
	void ForceClientAdjustment();
	
	virtual void OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity, FMovementBaseInterfaceData* NewMovementBaseInterfaceData, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode, FVector ServerGravityDirection) override;

	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;
private:
	bool CanDash() const;
	void Dash();
	void BunnyHop();
	
	FString DebugMove() const;
	
	bool bWantsToDash = false;
	bool bPendingBunnyHop = false;
	
	FASMoveState MoveState;
	
	float WallHitSpeedKept = 1.f;
	
	FASCharacterMoveResponseDataContainer MoveResponseContainer;
	FASCharacterNetworkMoveDataContainer MoveDataContainer;
};
