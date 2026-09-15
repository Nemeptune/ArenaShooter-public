// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Pickups/ASAmmoReceiver.h"
#include "Weapon/ASWeaponHolder.h"
#include "ASCharacter.generated.h"


class UASEquipmentComponent;
class UASCombatAttributeSet;
class UASAbilitySystemComponent;
class UGameplayEffect;
class UASGameplayAbility;
class UCameraComponent;
class USpringArmComponent;

USTRUCT()
struct FReplicatedDeathState
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsDead = false;

	UPROPERTY()
	FVector_NetQuantizeNormal ImpulseDir = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantize10 ImpulseLocation = FVector::ZeroVector;

	UPROPERTY()
	FName ImpulseBone = NAME_None;
};

UCLASS()
class ARENASHOOTER_API AASCharacter : public ACharacter, public IAbilitySystemInterface, public IASWeaponHolder, public IASAmmoReceiver
{
	GENERATED_BODY()
	
public:
	// ===========================================================
	//  Construction & engine lifecycle
	// ===========================================================

	AASCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override; 	// Only called on the Server. Calls before Server's AcknowledgePossession.
	virtual void OnRep_PlayerState() override; 	// Client only
	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override; 	// Implement IAbilitySystemInterface
	
	// ===========================================================
	//  Ability System & Attributes
	// ===========================================================

	UFUNCTION(BlueprintCallable, Category = "ASCharacter")
	int32 GetAbilityLevel() const;

	bool IsAlive() const;

	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|ASCharacter|Attributes")
	float GetHealth() const;
	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|ASCharacter|Attributes")
	float GetMaxHealth() const;
	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|ASCharacter|Attributes")
	float GetShield() const;
	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|ASCharacter|Attributes")
	float GetMaxShield() const;

	void RemoveCharacterAbilities(); // Server

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArenaShooter|Abilities")
	TObjectPtr<UASAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArenaShooter|Abilities")
	TObjectPtr<UASCombatAttributeSet> CombatAttributes;

	// Default abilities for this Character. These will be removed on Character death and regiven if Character respawns.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ArenaShooter|Abilities")
	TArray<TSubclassOf<UASGameplayAbility>> CharacterAbilities;

	// Ability that coordinates death. Granted once and kept across respawns (it must survive the
	// stripping of CharacterAbilities). Triggered by the Event.Death gameplay event.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ArenaShooter|Abilities")
	TSubclassOf<UASGameplayAbility> DeathAbility;
	
	// This is an instant GE that overrides the values for attributes that get reset on spawn/respawn.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ArenaShooter|Abilities")
	TSubclassOf<UGameplayEffect> DefaultAttributes;

	// Grant abilities on the Server. The Ability Specs will be replicated to the owning client.
	virtual void AddCharacterAbilities();

	// Grants the (persistent) death ability on the Server, once per ASC.
	void AddDeathAbility();
	virtual void InitializeAttributes();

	// Only for respawn — otherwise change attributes via a GE. These set the base value.
	virtual void SetHealth(float Health);
	virtual void SetShield(float Health);

public:
	virtual USkeletalMeshComponent* GetHolderMesh1P() const override;
	virtual USkeletalMeshComponent* GetHolderMesh3P() const override;
 
	virtual FName GetWeapon1PAttachPoint() const override;
	virtual FName GetWeapon3PAttachPoint() const override;
	
	virtual int32 GiveAmmo(FGameplayTag AmmoType, int32 Amount) override;
	
	// ===========================================================
	//  Death & ragdoll
	// ===========================================================
	
	void StartDeath(const FVector& ImpulseDir, const FVector& ImpulseLocation, FName ImpulseBone); // Server

protected:
	
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Death")
	float DeathImpulseStrength = 30000.f;
	void ApplyDeathImpulse();
	void SetRagdollPhysics(); // Runs on server and on every client via OnRep_DeathState.
	// Replicated to everyone so simulated proxies (and late/lagging clients) ragdoll on catch-up.
	UPROPERTY(ReplicatedUsing = OnRep_DeathState)
	FReplicatedDeathState DeathState;
	UFUNCTION()
	void OnRep_DeathState();
	
private:
	
	// ===========================================================
	//  Components
	// ===========================================================

	/** pawn mesh: 1st person view */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArenaShooter|Mesh", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh1P;
	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArenaShooter|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArenaShooter|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArm;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArenaShooter|Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UASEquipmentComponent> EquipmentComponent;

	// ===========================================================
	//  Weapon attach sockets & anim layers
	// ===========================================================

	UPROPERTY(EditDefaultsOnly, Category="ArenaShooter|Weapon|Attach", meta=(GetOptions="Get1PSockets"))
	FName WeaponAttachSocket1P = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category="ArenaShooter|Weapon|Attach", meta=(GetOptions="Get3PSockets"))
	FName WeaponAttachSocket3P = NAME_None;

	UFUNCTION()
	TArray<FName> Get1PSockets() const;
	UFUNCTION()
	TArray<FName> Get3PSockets() const;

	// ===========================================================
	//  Input
	// ===========================================================

	// TODO: make actions as abilities
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Input", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UInputAction>> SelectSlotActions; 
	
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArenaShooter|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArenaShooter|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArenaShooter|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArenaShooter|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TurnAction;
	
	UFUNCTION()
	void Move(const FInputActionValue& Value);
	UFUNCTION()
	void LookUp(const FInputActionValue& Value);
	UFUNCTION()
	void Turn(const FInputActionValue& Value);
	UFUNCTION()
	void HandleSelectSlot(int32 Slot);
	
	// ===========================================================
	//  Animation
	// ===========================================================
public:
	virtual void LinkAnimLayers(TSubclassOf<UAnimInstance> FPLayer, TSubclassOf<UAnimInstance> TPLayer) override;
	
private:
	
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Animation", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UAnimInstance> UnarmedFPLayer;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Animation", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UAnimInstance> UnarmedTPLayer;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> CurrentFPLayer;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> CurrentTPLayer;

	static void SwapLayer(USkeletalMeshComponent* Mesh, TSubclassOf<UAnimInstance>& Current, TSubclassOf<UAnimInstance> Wanted);
	
};
