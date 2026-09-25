// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ASCharacterMovementComponent.h"

#include "GameFramework/Character.h"
#include "System/ASLogChannels.h"
#include "System/ASProfiling.h"
#include "Weapon/ASLagCompensationSubsystem.h"

UASCharacterMovementComponent::UASCharacterMovementComponent()
{
	SetMoveResponseDataContainer(MoveResponseContainer);
	SetNetworkMoveDataContainer(MoveDataContainer);
}

void FASCharacterMoveResponseDataContainer::ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment)
{
	Super::ServerFillResponseData(CharacterMovement, PendingAdjustment);
	MoveState = CastChecked<const UASCharacterMovementComponent>(&CharacterMovement)->GetMoveState();
}

bool FASCharacterMoveResponseDataContainer::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap)
{
	if (!Super::Serialize(CharacterMovement, Ar, PackageMap))
	{
		return false;
	}
	
	if (IsCorrection())
	{
		MoveState.Serialize(Ar);
	}
	
	return !Ar.IsError();
}

void FASCharacterNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	Super::ClientFillNetworkMoveData(ClientMove, MoveType);
	ViewServerTime = static_cast<const FSavedMove_ASCharacter&>(ClientMove).GetViewServerTime();
}

bool FASCharacterNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);
	Ar << ViewServerTime;
	return !Ar.IsError();
}

void FSavedMove_ASCharacter::Clear()
{
	Super::Clear();
	bSavedWantsToDash = false;
	StartMoveState = FASMoveState();
	SavedViewServerTime = 0.;
}

void FSavedMove_ASCharacter::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	bSavedWantsToDash = GetMovement(C)->bWantsToDash;
	const UASLagCompensationSubsystem* LagCompensation = C->GetWorld()->GetSubsystem<UASLagCompensationSubsystem>();
	SavedViewServerTime = LagCompensation ? LagCompensation->GetShownServerTime() : 0.;
}

void FSavedMove_ASCharacter::SetInitialPosition(ACharacter* C)
{
	Super::SetInitialPosition(C);
	StartMoveState = GetMovement(C)->MoveState;
}

uint8 FSavedMove_ASCharacter::GetCompressedFlags() const
{
	uint8 Flags = Super::GetCompressedFlags();
	if (bSavedWantsToDash)
	{
		Flags |= FLAG_Custom_0;
	}
	
	return Flags;
}

void FSavedMove_ASCharacter::CombineWith(const FSavedMove_Character* OldMove, ACharacter* InCharacter, APlayerController* PC, const FVector& OldStartLocation)
{
	Super::CombineWith(OldMove, InCharacter, PC, OldStartLocation);
	
	GetMovement(InCharacter)->MoveState = static_cast<const FSavedMove_ASCharacter*>(OldMove)->StartMoveState;
}

double FSavedMove_ASCharacter::GetViewServerTime() const
{
	return SavedViewServerTime;
}

UASCharacterMovementComponent* FSavedMove_ASCharacter::GetMovement(const ACharacter* Character)
{
	return CastChecked<UASCharacterMovementComponent>(Character->GetCharacterMovement());
}

FSavedMovePtr FNetworkPredictionData_Client_ASCharacter::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_ASCharacter());
}

void UASCharacterMovementComponent::AddDampedImpulse(FVector Impulse, bool bSelfInflicted)
{
	if (!HasValidData() || Impulse.IsZero())
	{
		return;
	}

	FVector FinalImpulse = Impulse;
	if (Mass > UE_SMALL_NUMBER)
	{
		FinalImpulse /= Mass;
	}

	/* Horizontal pass: split the impulse into the part running along our current velocity and the part
	 * orthogonal to it, then progressively damp the former the faster we're already going.
	 * Standing still leaves XYDelta at zero, so the first explosion always lands undamped. */
	const float FinalImpulseZ = FinalImpulse.Z;
	FinalImpulse.Z = 0.f;

	const FVector PendingVelocity = Velocity + PendingImpulseToApply;
	const FVector PendingVelocityDir = PendingVelocity.GetSafeNormal();
	const FVector AdditiveImpulse = PendingVelocityDir * (PendingVelocityDir | FinalImpulse);
	const FVector OrthogonalImpulse = FinalImpulse - AdditiveImpulse;
	const float CurrentXYSpeed = PendingVelocity.Size2D();
	const float XYDelta = (PendingVelocity + AdditiveImpulse).Size2D() - CurrentXYSpeed;

	if (XYDelta > 0.f)
	{
		const float AboveDampFactor = (KnockbackDampingSpeed > UE_SMALL_NUMBER) ? (CurrentXYSpeed / KnockbackDampingSpeed) : 0.f;
		if (AboveDampFactor > 1.f)
		{
			FinalImpulse = AdditiveImpulse / AboveDampFactor + OrthogonalImpulse;
		}

		const float PctBelowRun = FMath::Clamp((MaxWalkSpeed - CurrentXYSpeed) / XYDelta, 0.f, 1.f);
		float PctBelowDamp = FMath::Clamp((KnockbackDampingSpeed - CurrentXYSpeed) / XYDelta, 0.f, 1.f);
		const float PctAboveDamp = FMath::Max(0.f, 1.f - PctBelowDamp);
		PctBelowDamp = FMath::Max(0.f, PctBelowDamp - PctBelowRun);

		FinalImpulse *= (PctBelowRun + PctBelowDamp + FMath::Max(0.5f, 1.f - PctAboveDamp) * PctAboveDamp);

		FVector FinalVelocityXY = PendingVelocity + FinalImpulse;
		FinalVelocityXY.Z = 0.f;
		if (FinalVelocityXY.Size() > MaxKnockbackHorizontalVelocity)
		{
			FinalImpulse = FinalVelocityXY.GetSafeNormal() * MaxKnockbackHorizontalVelocity - PendingVelocity;
		}
	}

	FinalImpulse.Z = FinalImpulseZ;

	// Vertical pass: self-inflicted impulses get a higher undamped ceiling, so your own rocket lifts you
	// further than someone else's does.
	const float DampingThreshold = bSelfInflicted ? MaxUndampedImpulseZ : MaxAdditiveKnockbackZ;
	if (FinalImpulse.Z > 0.f && (FinalImpulse.Z + PendingVelocity.Z) > DampingThreshold)
	{
		const float PctBelowBoost = FMath::Clamp((DampingThreshold - PendingVelocity.Z) / FinalImpulse.Z, 0.f, 1.f);
		FinalImpulse.Z *= (PctBelowBoost + (1.f - PctBelowBoost) * FMath::Max(0.5f, PctBelowBoost));
	}

	PendingImpulseToApply += FinalImpulse;

	ForceClientAdjustment();
}

void UASCharacterMovementComponent::RequestDash()
{
	bWantsToDash = true;
}

const FASMoveState& UASCharacterMovementComponent::GetMoveState() const
{
	return MoveState;
}

FNetworkPredictionData_Client* UASCharacterMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UASCharacterMovementComponent* MutableThis = const_cast<UASCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_ASCharacter(*this);
	}
	
	return ClientPredictionData;
}

bool UASCharacterMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	const bool bWasOnGround = IsMovingOnGround();
	if (!Super::DoJump(bReplayingMoves, DeltaTime))
	{
		return false;
	}
	
	bPendingBunnyHop = bWasOnGround;
	return true;
}

void UASCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
	
	// SimulateMovement calls this too. Proxies get the result through replicated movement, never the input.
	if (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return;
	}
	
	MoveState.DashCooldownRemaining = FMath::Max(0.f, MoveState.DashCooldownRemaining - DeltaSeconds);
	MoveState.BunnyHopGainCooldown = FMath::Max(0.f, MoveState.BunnyHopGainCooldown - DeltaSeconds);
	
	if (bPendingBunnyHop)
	{
		bPendingBunnyHop = false;
		BunnyHop();
	}
	
	if (bWantsToDash)
	{
		bWantsToDash = false;
		if (CanDash())
		{
			Dash();
		}
	}
}

void UASCharacterMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);
	
	if (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return;
	}
	
	if (WallHitSpeedKept < 1.f)
	{
		const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.f);
		const FVector::FReal Speed = HorizontalVelocity.Size();
		if (Speed > MaxWalkSpeed)
		{
			// Only the speed earned above running speed is damped, so a hard hit ends the chain without a dead stop.
			const FVector::FReal NewSpeed = MaxWalkSpeed + (Speed - MaxWalkSpeed) * WallHitSpeedKept;
			Velocity.X = HorizontalVelocity.X * (NewSpeed / Speed);
			Velocity.Y = HorizontalVelocity.Y * (NewSpeed / Speed);
		}
		
		MoveState.BunnyHopSpeed = WallHitSpeedKept > 0.f ? FMath::Min(MoveState.BunnyHopSpeed, static_cast<float>(Velocity.Size2D())) : 0.f;
		WallHitSpeedKept = 1.f;
		
		UE_LOG(LogAS_Movement, Verbose, TEXT("%s Wall hit: kept %.2f, speed %.0f -> %.0f, chain -> %.0f"),
		*DebugMove(), WallHitSpeedKept, Speed, Velocity.Size2D(), MoveState.BunnyHopSpeed);
	}
	
	if (!IsMovingOnGround())
	{
		MoveState.TimeOnGround = 0.f;
		return;
	}
	
	MoveState.TimeOnGround += DeltaSeconds;
	if (MoveState.TimeOnGround > BunnyHopWindow && MoveState.BunnyHopSpeed > 0.f)
	{
		UE_LOG(LogAS_Movement, Verbose, TEXT("%s Chain ended: %.2f s on the ground"), *DebugMove(), MoveState.TimeOnGround);
		MoveState.BunnyHopSpeed = 0.f;
	}
}

void UASCharacterMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	Super::HandleImpact(Hit, TimeSlice, MoveDelta);
	
	if (MoveState.BunnyHopSpeed <= 0.f || IsWalkable(Hit))
	{
		return;
	}
	if (Cast<APawn>(Hit.GetActor()))
	{
		return;
	}
	
	const FVector MoveDirection = Velocity.GetSafeNormal2D();
	const FVector WallNormal = Hit.Normal.GetSafeNormal2D();
	
	const float HeadOn = FMath::Clamp(static_cast<float>(-(MoveDirection | WallNormal)), 0.f, 1.f);
	const float ImpactAngle = FMath::RadiansToDegrees(FMath::Asin(HeadOn));
	
	const float Kept = BunnyHopWallBreakAngle > 0.f ? FMath::Clamp(1.f - ImpactAngle / BunnyHopWallBreakAngle, 0.f, 1.f) : 0.f;
	
	WallHitSpeedKept = FMath::Min(WallHitSpeedKept, Kept);
}

void UASCharacterMovementComponent::BunnyHop()
{
	const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.f);
	const FVector::FReal CurrentSpeed = HorizontalVelocity.Size();
	
	FVector Direction = Acceleration.GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		Direction = HorizontalVelocity.GetSafeNormal();
	}
	if (Direction.IsNearlyZero())
	{
		// Jumping in place doesn't start a chain.
		MoveState.BunnyHopSpeed = 0.f;
		return;
	}
	
	// The chain remembers its speed, so speed lost steering in the air doesn't cost the next hop.
	const FVector::FReal EarnedSpeed = FMath::Max<FVector::FReal>(CurrentSpeed, MoveState.BunnyHopSpeed);
	// A hop right after the previous one (an up-ramp lands it again in the same move) keeps the chain's speed but adds nothing.
	const float Gain = MoveState.BunnyHopGainCooldown <= 0.f ? BunnyHopSpeedGain : 0.f;
	const FVector::FReal NewSpeed = FMath::Max<FVector::FReal>(FMath::Min<FVector::FReal>(EarnedSpeed + Gain, BunnyHopMaxSpeed), CurrentSpeed);
	
	Velocity.X = Direction.X * NewSpeed;
	Velocity.Y = Direction.Y * NewSpeed;
	MoveState.BunnyHopSpeed = NewSpeed;
	MoveState.BunnyHopGainCooldown = BunnyHopMinGainInterval;
	
	UE_LOG(LogAS_Movement, Verbose, TEXT("%s Hop: %.0f -> %.0f cm/s, dir (%.2f, %.2f), gain %.0f"),
		*DebugMove(), CurrentSpeed, NewSpeed, Direction.X, Direction.Y, Gain);
}

FString UASCharacterMovementComponent::DebugMove() const
{
	float TimeStamp = 0.f;
	if (const FCharacterNetworkMoveData* MoveData = GetCurrentNetworkMoveData())
	{
		TimeStamp = MoveData->TimeStamp;
	}
	else if (ClientPredictionData)
	{
		TimeStamp = ClientPredictionData->CurrentTimeStamp;
	}

	const TCHAR* Side = CharacterOwner->HasAuthority() ? TEXT("Server") : (CharacterOwner->bClientUpdating ? TEXT("Client replay") : TEXT("Client"));
	return FString::Printf(TEXT("%s %s T=%.3f"), *GetNameSafe(CharacterOwner), Side, TimeStamp);
}

void UASCharacterMovementComponent::ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration)
{
	// Landing inside the window of a running chain keeps its speed; friction resumes once the window closes.
	if (MoveState.BunnyHopSpeed > 0.f && IsMovingOnGround() && MoveState.TimeOnGround <= BunnyHopWindow)
	{
		return;
	}
	
	Super::ApplyVelocityBraking(DeltaTime, Friction, BrakingDeceleration);
}

void UASCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	
	bWantsToDash = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

bool UASCharacterMovementComponent::CanDash() const
{
	return MoveState.DashCooldownRemaining <=0.f && (IsMovingOnGround() || IsFalling());
}

void UASCharacterMovementComponent::Dash()
{
	// The direction comes from this move's acceleration, which the server receives with the same move. No input means forward.
	FVector DashDirection = Acceleration.GetSafeNormal2D();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
	}
	
	const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.f);
	const FVector::FReal AlongSpeed = HorizontalVelocity | DashDirection;
	const FVector SidewaysVelocity = HorizontalVelocity - DashDirection * AlongSpeed;
	const FVector NewHorizontalVelocity = DashDirection * FMath::Max<FVector::FReal>(AlongSpeed, DashSpeed) + SidewaysVelocity;
	
	Velocity = FVector(NewHorizontalVelocity.X, NewHorizontalVelocity.Y, FMath::Max<FVector::FReal>(Velocity.Z, DashLiftSpeed));
	SetMovementMode(MOVE_Falling);
	
	MoveState.DashCooldownRemaining = DashCooldown;
}

float UASCharacterMovementComponent::GetDashCooldownRemaining() const
{
	return MoveState.DashCooldownRemaining;
}

bool UASCharacterMovementComponent::ClientUpdatePositionAfterServerUpdate()
{
	// The replay re-applies the old moves' flags and consumes them, which would also wipe a press made this frame.
	const bool bRealWantsToDash = bWantsToDash;
	const bool bReplayed = Super::ClientUpdatePositionAfterServerUpdate();
	bWantsToDash = bRealWantsToDash;
	return bReplayed;
}

void UASCharacterMovementComponent::ForceClientAdjustment()
{
	if (CharacterOwner && CharacterOwner->HasAuthority() && CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy)
	{
		if (FNetworkPredictionData_Server_Character* ServerData = GetPredictionData_Server_Character())
		{
			ServerData->bForceClientUpdate = true;
		}
	}
}

void UASCharacterMovementComponent::OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity, FMovementBaseInterfaceData* NewMovementBaseInterfaceData, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode, FVector ServerGravityDirection)
{
	CSV_CUSTOM_STAT(ArenaShooter, MoveCorrections, 1, ECsvCustomStatOp::Accumulate);
	
	MoveState = static_cast<const FASCharacterMoveResponseDataContainer&>(GetMoveResponseDataContainer()).MoveState;
	
	Super::OnClientCorrectionReceived(ClientData, TimeStamp, NewLocation, NewVelocity, NewMovementBaseInterfaceData, NewBaseBoneName, bHasBase, bBaseRelativePosition, ServerMovementMode, ServerGravityDirection);
}

void UASCharacterMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel)
{
	// Server: the move being processed carries the moment its client was seeing. ServerMove applied its
	// ControlRotation just before this, so a shot's aim and rewind time come from the same move.
	// Client replays also set the current move data, hence the authority check.
	const FCharacterNetworkMoveData* MoveData = GetCurrentNetworkMoveData();
	if (MoveData && GetOwnerRole() == ROLE_Authority)
	{
		if (UASLagCompensationSubsystem* LagCompensation = GetWorld()->GetSubsystem<UASLagCompensationSubsystem>())
		{
			LagCompensation->NoteClientViewTime(CharacterOwner, static_cast<const FASCharacterNetworkMoveData*>(MoveData)->ViewServerTime);
		}
	}

	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}
