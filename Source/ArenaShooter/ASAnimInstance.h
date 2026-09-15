// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Animation/AnimInstance.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/ASCharacterMovementComponent.h"
#include "ASAnimInstance.generated.h"

class UASCharacterMovementComponent;

UENUM(BlueprintType)
enum class ECardinalDirection : uint8
{
	Forward UMETA(DisplayName = "Forward"),
	Backward UMETA(DisplayName = "Backward"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right")
};

UENUM(BlueprintType)
enum class ERootYawOffsetMode : uint8
{
	BlendOut UMETA(DisplayName = "BlendOut"),
	Hold UMETA(DisplayName = "Hold"),
	Accumulate UMETA(DisplayName = "Accumulate")
};

struct FAnimCharacterState
{
	FVector WorldLocation = FVector::ZeroVector;
	FRotator WorldRotation = FRotator::ZeroRotator;
	FVector Velocity = FVector::ZeroVector;
	FVector Acceleration = FVector::ZeroVector;
	FRotator BaseAimRotation = FRotator::ZeroRotator;
	bool bIsMovingOnGround = true;
	EMovementMode MovementMode = MOVE_Walking;
	float GravityZ = -980.f;
};

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsFirstTick = true;
	
protected:

	FAnimCharacterState CharacterState;
	
	// Location data
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DisplacementSincelastUpdate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DisplacementSpeed;

	// Rotation data
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator WorldRotation;
	FRotator MovementRotation = FRotator::ZeroRotator;
	FRotator ActorRotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float YawDeltaSinceLastUpdate;
	float YawDeltaSpeed;

	// Velocity data

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector WorldVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector LocalVelocity2D;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float LocalVelocityDirectionAngle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float LocalVelocityDirectionAngleWithOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECardinalDirection LocalVelocityDirection = ECardinalDirection::Forward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECardinalDirection LocalVelocityDirectionNoOffset = ECardinalDirection::Forward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool HasVelocity = false;

	// Acceleration data
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector LocalAcceleration2D;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool HasAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECardinalDirection PivotInitialDirection = ECardinalDirection::Forward;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LastPivotTime;
	FVector PivotDirection2D;


	// Character state data
	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	float GroundDistance = -1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	bool IsOnGround;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	bool IsCrouching = false; //can't crouch for now
	bool CrouchStateChange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	float TimeSinceFiredWeapon = 9999.f;

	// in case we need more tags - store them in tag map
	UPROPERTY(EditDefaultsOnly, Category="GameplayTags")
	FGameplayTag FireTag;

	void OnFireTagChanged(FGameplayTag Tag, int32 NewCount);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character State Data")
	bool bIsFiring;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character State Data")
	bool bIsJumping;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character State Data")
	bool bIsFalling;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character State Data")
	float TimeToJumpApex = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	bool IsRunningIntoWall = false;

	// Locomotion SM Data
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECardinalDirection CardinalDirectionFromAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECardinalDirection StartDirection;

	//Blend Weight data
	float UpperbodyDynamicAdditive = 0.f;

	//Aiming data
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double AimPitch = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double AimYaw = 0.f;

	// Settings
	float CardinalDirectionDeadZone = 10.f;

	// Turn In Place

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float RootYawOffset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FFloatSpringState RootYawOffsetSpringState;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float TurnYawCurveValue;
	
	float PreviousTurnYawCurveValue;
	FName TurnYawWeightName = TEXT("TurnYawWeight");
	FName RemainingTurnYawName = TEXT("RemainingTurnYaw");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	ERootYawOffsetMode RootYawOffsetMode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D RootYawOffsetAngleClamp{ -120.f, 100.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D RootYawOffsetAngleClampCrouched{ -90.f, 80.f };

	bool bEnableRootYawOffset = true;

private:

	//Thread safe update functions
	UFUNCTION(BlueprintCallable, Category = "Updates", meta = (BlueprintThreadSafe))
	void UpdateLocationData(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Updates", meta = (BlueprintThreadSafe))
	void UpdateRotationData(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Updates", meta = (BlueprintThreadSafe))
	void UpdateVelocityData();

	UFUNCTION(BlueprintCallable, Category = "Updates", meta = (BlueprintThreadSafe))
	void UpdateAccelerationData();

	UFUNCTION(BlueprintCallable, Category = "Updates", meta = (BlueprintThreadSafe))
	void UpdateWallDetectionHeuristic(); 

	UFUNCTION(BlueprintCallable, Category = "Updates", meta = (BlueprintThreadSafe))
	void UpdateCharacterStateData(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Updates", meta = (BlueprintThreadSafe))
	void UpdateBlendWeightData(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Updates", meta = (BlueprintThreadSafe))
	void UpdateRootYawOffset(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Updates", meta = (BlueprintThreadSafe))
	void UpdateAimingData();

	UFUNCTION(BlueprintCallable, Category = "Updates", meta = (BlueprintThreadSafe))
	void UpdateJumpFallData();

	UFUNCTION(BlueprintCallable, Category = "TurnInPlace", meta = (BlueprintThreadSafe))
	void ProcessTurnYawCurve();

	UFUNCTION(BlueprintCallable, Category = "Settings", meta = (BlueprintThreadSafe))
	FORCEINLINE UASCharacterMovementComponent* GetCharacterMovement() const
	{
		return Cast<UASCharacterMovementComponent>(TryGetPawnOwner()->GetMovementComponent());
	}

	//helper functions
	UFUNCTION(meta = (BlueprintThreadSafe))
	ECardinalDirection SelectCardinalDirectionfromAngle(float Angle, float DeadZone, ECardinalDirection CurrentDirection = ECardinalDirection::Forward, bool bUseCurrentDirection = false); // not sure about false
	
	UFUNCTION(meta = (BlueprintThreadSafe))
	ECardinalDirection GetOppositeCardinalDirection(ECardinalDirection CurrentDirection) const;
	
	UFUNCTION(BlueprintPure, Category = "Updates", meta = (BlueprintThreadSafe))
	bool IsMovingPerpendicularToInitialPivot();

	//Turn in place
	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	void SetRootYawOffset(float InRootYawOffset);
};
