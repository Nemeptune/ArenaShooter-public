// Fill out your copyright notice in the Description page of Project Settings.


#include "ASAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "KismetAnimationLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"

void UASAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (AActor* Owner = GetOwningActor())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner))
		{
			InitializeWithAbilitySystem(ASC);
		}
	}
}

void UASAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	APawn* Pawn = TryGetPawnOwner();
	if (!Pawn) return;
	
	CharacterState.WorldLocation = Pawn->GetActorLocation();
	CharacterState.WorldRotation = Pawn->GetActorRotation();
	CharacterState.Velocity = Pawn->GetVelocity();
	CharacterState.BaseAimRotation = Pawn->GetBaseAimRotation();

	if (UASCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		CharacterState.Acceleration = Movement->GetCurrentAcceleration();
		CharacterState.bIsMovingOnGround = Movement->IsMovingOnGround();
		CharacterState.MovementMode = Movement->MovementMode;
		CharacterState.GravityZ = Movement->GetGravityZ(); // check if it even in use
	}
}

void UASAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	UpdateLocationData(DeltaSeconds);
	UpdateRotationData(DeltaSeconds);
	UpdateVelocityData();
	UpdateAccelerationData();
	UpdateWallDetectionHeuristic();
	UpdateCharacterStateData(DeltaSeconds);
	UpdateBlendWeightData(DeltaSeconds);
	UpdateRootYawOffset(DeltaSeconds);
	UpdateAimingData();
	UpdateJumpFallData();
	
	if (bIsFirstTick)
	{
		bIsFirstTick = false;
	}
}

void UASAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);
	if (FireTag.IsValid())
	{
		ASC->RegisterAndCallGameplayTagEvent(FireTag,FOnGameplayEffectTagCountChanged::FDelegate::CreateUObject(this, & UASAnimInstance::OnFireTagChanged));
	}
}

void UASAnimInstance::OnFireTagChanged(FGameplayTag Tag, int32 NewCount)
{
	bIsFiring = (NewCount > 0);
}

void UASAnimInstance::UpdateLocationData(float DeltaSeconds)
{
	FVector Location = CharacterState.WorldLocation ;
	DisplacementSincelastUpdate = (Location - WorldLocation).Length();
	WorldLocation = Location;
	DisplacementSpeed = UKismetMathLibrary::SafeDivide(DisplacementSincelastUpdate, DeltaSeconds);
	if (bIsFirstTick)
	{
		DisplacementSincelastUpdate = 0.f;
		DisplacementSpeed = 0;
	}	
}

void UASAnimInstance::UpdateRotationData(float DeltaSeconds)
{
	FRotator Rotation = CharacterState.WorldRotation;
	
	YawDeltaSinceLastUpdate = Rotation.Yaw - WorldRotation.Yaw;
	YawDeltaSpeed = UKismetMathLibrary::SafeDivide(YawDeltaSinceLastUpdate ,DeltaSeconds);
	WorldRotation = Rotation;
	
	if (bIsFirstTick)
	{
		YawDeltaSinceLastUpdate = 0.f;
	}
}

void UASAnimInstance::UpdateVelocityData()
{
	bool WasMovingLastUpdate = !LocalVelocity2D.IsZero();
	
	WorldVelocity = CharacterState.Velocity;

	FVector WorldVelocity2D = FVector(WorldVelocity.X, WorldVelocity.Y, 0.f);
	LocalVelocity2D = WorldRotation.UnrotateVector(WorldVelocity2D);
	
	LocalVelocityDirectionAngle = UKismetAnimationLibrary::CalculateDirection(WorldVelocity2D, WorldRotation);
	LocalVelocityDirectionAngleWithOffset = LocalVelocityDirectionAngle - RootYawOffset;
	
	LocalVelocityDirection = SelectCardinalDirectionfromAngle(LocalVelocityDirectionAngleWithOffset, CardinalDirectionDeadZone, LocalVelocityDirection, WasMovingLastUpdate);
	LocalVelocityDirectionNoOffset = SelectCardinalDirectionfromAngle(LocalVelocityDirectionAngle, CardinalDirectionDeadZone, LocalVelocityDirectionNoOffset, WasMovingLastUpdate);
	
	HasVelocity = LocalVelocity2D.SquaredLength() > KINDA_SMALL_NUMBER;
}

void UASAnimInstance::UpdateAccelerationData()
{
	FVector WorldAcceleration2D = CharacterState.Acceleration;
	WorldAcceleration2D.Z = 0.f;

	LocalAcceleration2D = WorldRotation.UnrotateVector(WorldAcceleration2D);
	HasAcceleration = LocalAcceleration2D.SquaredLength() > KINDA_SMALL_NUMBER;;

	//Calculate a cardinal direction from acceleration to be used for pivots. 
	//Acceleration communicates player intent better for that purpose than velocity does.

	PivotDirection2D = FMath::Lerp(PivotDirection2D, WorldAcceleration2D.GetSafeNormal(), 0.5).GetSafeNormal();
	
	float Angle = UKismetAnimationLibrary::CalculateDirection(PivotDirection2D, WorldRotation);
	CardinalDirectionFromAcceleration = GetOppositeCardinalDirection(SelectCardinalDirectionfromAngle(Angle, CardinalDirectionDeadZone));
}

/*This logic guesses if the character is running into a wall by checking
 if there's a large angle between velocity and acceleration 
 (i.e. the character is pushing towards the wall but actually sliding to the side) 
 and if the character is trying to accelerate but speed is relatively low. */
void UASAnimInstance::UpdateWallDetectionHeuristic()
{
	//float Dot = FVector::DotProduct(FVector(LocalAcceleration2D.GetSafeNormal(0.00001)), FVector(LocalVelocity2D.GetSafeNormal(0.00001)));
	//IsRunningIntoWall = LocalAcceleration2D.Length() > 0.1 || LocalVelocity2D.Length() < 200.0 || (Dot >= -0.6 || Dot <= 0.6);
}

void UASAnimInstance::UpdateCharacterStateData(float DeltaSeconds)
{
	IsOnGround = CharacterState.bIsMovingOnGround;
	
	// TODO update crouch state //
	// TODO is firing //
	TimeSinceFiredWeapon += DeltaSeconds;
	if (bIsFiring)
	{
		TimeSinceFiredWeapon = 0.f;
	}
	
	bIsJumping = false;
	bIsFalling = false;
	
	if (CharacterState.MovementMode == EMovementMode::MOVE_Falling)
	{
		if (WorldVelocity.Z > 0.f)
		{
			bIsJumping = true;
		}
		else
		{
			bIsFalling = true;
		}
	}
}

void UASAnimInstance::UpdateBlendWeightData(float DeltaSeconds)
{
	// float BValue = FMath::FInterpTo(UpperbodyDynamicAdditive, 0.f, DeltaSeconds, 6.0);
	// UpperbodyDynamicAdditive = UKismetMathLibrary::SelectFloat(1.0, BValue, IsAnyMontagePlaying() || IsOnGround);
	float Target = (IsAnyMontagePlaying() || IsOnGround) ? 1.0f : 0.0f;
	UpperbodyDynamicAdditive = FMath::FInterpTo(UpperbodyDynamicAdditive, Target, DeltaSeconds, 6.0f);
}

//This function handles updating the yaw offset depending on the current state of the Pawn owner.
void UASAnimInstance::UpdateRootYawOffset(float DeltaSeconds)
{
	// When the feet aren't moving (e.g. during Idle), offset the root in the opposite direction 
	// to the Pawn owner's rotation to keep the mesh from rotating with the Pawn.
	if (RootYawOffsetMode == ERootYawOffsetMode::Accumulate)
	{
		SetRootYawOffset(RootYawOffset - YawDeltaSinceLastUpdate);

	}
	else if (RootYawOffsetMode == ERootYawOffsetMode::BlendOut)
	{
		float InterpFloat = UKismetMathLibrary::FloatSpringInterp(RootYawOffset, 0.f, RootYawOffsetSpringState, 80.f, 1.f, DeltaSeconds, 1.f, 0.5);
		SetRootYawOffset(InterpFloat);
	}
	/*Reset to blending out the yaw offset. Each update, a state needs to request to accumulate or hold the offset. 
	Otherwise, the offset will blend out. This is primarily because the majority of states want the offset to blend out, 
	so this saves on having to tag each state.
	*/
	RootYawOffsetMode = ERootYawOffsetMode::BlendOut;
}

//When the yaw offset gets too big, we trigger TurnInPlace animations to rotate the character back.
void UASAnimInstance::ProcessTurnYawCurve()
{
	//The "TurnYawWeight" curve is set to 1 in TurnInPlace animations, so its current value from GetCurveValue will be the current weight of the TurnInPlace animation. 
	//We can use this to "unweight" the TurnInPlace animation to get the full RemainingTurnYaw curve value.
	PreviousTurnYawCurveValue = TurnYawCurveValue;
	if (FMath::IsNearlyEqual(GetCurveValue(TurnYawWeightName),0))
	{
		TurnYawCurveValue = 0.f;
		PreviousTurnYawCurveValue = 0.f;
	}
	else
	{
		float TurnYawWeight = GetCurveValue(TurnYawWeightName);
		float RemainingTurnYaw = GetCurveValue(RemainingTurnYawName);
		
		TurnYawCurveValue = RemainingTurnYaw / TurnYawWeight;
		//Avoid applying the curve delta when the curve first becomes relevant.
		if (PreviousTurnYawCurveValue == 0.f) { return; }
		//Reduce the root yaw offset by the amount of rotation from the turn animation.
		float ProcessedRootYawOffset = RootYawOffset - (TurnYawCurveValue - PreviousTurnYawCurveValue);
		SetRootYawOffset(ProcessedRootYawOffset);
	}
}

void UASAnimInstance::UpdateAimingData()
{
	AimPitch = FRotator::NormalizeAxis(CharacterState.BaseAimRotation.Pitch);
}

void UASAnimInstance::UpdateJumpFallData()
{
	if (bIsJumping)
	{
		TimeToJumpApex = (0.f - WorldVelocity.Z) / CharacterState.GravityZ;
	}
	else
	{
		TimeToJumpApex = 0.f;
	}
}

ECardinalDirection UASAnimInstance::SelectCardinalDirectionfromAngle(float Angle, float DeadZone, ECardinalDirection CurrentDirection, bool bUseCurrentDirection)
{
	float AbsAngle = FMath::Abs(Angle);
	float FwdDeadZone = DeadZone;
	float BwdDeadZone = FwdDeadZone;
	
	if (bUseCurrentDirection)
	{
		switch (CurrentDirection)
		{
		case ECardinalDirection::Forward:
			FwdDeadZone *= 2.f;
			break;
		case ECardinalDirection::Backward:
			BwdDeadZone *= 2.f;
			break;
		default:
			break;
		}
	}

	if (AbsAngle <= (FwdDeadZone + 45.f))
	{
		return ECardinalDirection::Forward;
	}
	else if (AbsAngle >= (135.f - BwdDeadZone))
	{
		return ECardinalDirection::Backward;
	}
	else if (Angle > 0.f)
	{
		return ECardinalDirection::Right;
	}
	else
	{
		return ECardinalDirection::Left;
	}
}

ECardinalDirection UASAnimInstance::GetOppositeCardinalDirection(ECardinalDirection CurrentDirection) const
{
	switch (CurrentDirection)
	{
	case ECardinalDirection::Forward:
		return ECardinalDirection::Backward;
		break;
	case ECardinalDirection::Backward:
		return ECardinalDirection::Forward;
		break;
	case ECardinalDirection::Left:
		return ECardinalDirection::Right;
		break;
	case ECardinalDirection::Right:
		return ECardinalDirection::Left;
		break;
	default:
		return ECardinalDirection::Forward;
		break;
	}
}

bool UASAnimInstance::IsMovingPerpendicularToInitialPivot()
{
	//We stay in a pivot when pivoting along a line (e.g. triggering a left-right pivot while playing a right-left pivot),
	//but break out if the character makes a perpendicular change in direction.
	bool bVerticalPivot = (PivotInitialDirection == ECardinalDirection::Forward || PivotInitialDirection == ECardinalDirection::Backward);
	bool bVerticalVelocity = (LocalVelocityDirection == ECardinalDirection::Forward || LocalVelocityDirection == ECardinalDirection::Backward);
    
	bool bHorizontalPivot = (PivotInitialDirection == ECardinalDirection::Left || PivotInitialDirection == ECardinalDirection::Right);
	bool bHorizontalVelocity = (LocalVelocityDirection == ECardinalDirection::Left || LocalVelocityDirection == ECardinalDirection::Right);

	return (bVerticalPivot && !bVerticalVelocity) || (bHorizontalPivot && !bHorizontalVelocity);
}

void UASAnimInstance::SetRootYawOffset(float InRootYawOffset)
{
	if (bEnableRootYawOffset)
	{
		/*
		We clamp the offset because at large offsets the character has to aim too far backwards, which over twists the spine.
		The turn in place animations will usually keep up with the offset, but this clamp will cause the feet to slide if the user rotates the camera too quickly.
		If desired, this clamp could be replaced by having aim animations that can go up to 180 degrees or by triggering turn in place animations more aggressively.
		*/
		// Select the appropriate value based on bIsCrouchingf
		FVector2D SelectedValue = IsCrouching ? RootYawOffsetAngleClampCrouched : RootYawOffsetAngleClamp;
		float NormalizedOffset = FMath::UnwindDegrees(InRootYawOffset);
		float ClampedAngle = FMath::ClampAngle(NormalizedOffset, SelectedValue.X, SelectedValue.Y);
		RootYawOffset = SelectedValue.X == SelectedValue.Y ? NormalizedOffset : ClampedAngle;

		//We want aiming to counter the yaw offset to keep the weapon aiming in line with the camera.
		AimYaw = RootYawOffset * (-1.f);
	}
	else
	{
		//We want aiming to counter the yaw offset 
		// to keep the weapon aiming in line with the camera.
		RootYawOffset = 0.0;
		AimYaw = 0.f;
	}
}
