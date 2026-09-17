// Fill out your copyright notice in the Description page of Project Settings.


#include "ASCharacter.h"

#include "ASAnimInstance.h"
#include "AbilitySystem/ASAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ASCombatAttributeSet.h"
#include "System/ASGameplayTags.h"
#include "Settings/ASGameUserSettings.h"
#include "System/ASLogChannels.h"
#include "AbilitySystem/Abilities/ASGameplayAbility.h"
#include "Player/ASPlayerController.h"
#include "Player/ASPlayerState.h"
#include "Character/ASCharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Inventory/ASEquipmentComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Sound/SoundCue.h"

AASCharacter::AASCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UASCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeLocation(FVector(0, 50, 68.492264));
	SpringArm->bUsePawnControlRotation = false;

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(RootComponent);
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh1P"));
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionProfileName(FName("NoCollision"));
	Mesh1P->bReceivesDecals = false;
	Mesh1P->CastShadow = false;
	
	EquipmentComponent = CreateDefaultSubobject<UASEquipmentComponent>(TEXT("EquipmentComponent"));
}

void AASCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

//server only
void AASCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	AASPlayerState* PS = GetPlayerState<AASPlayerState>();
	if (PS)
	{
		AbilitySystemComponent = PS->GetASAbilityComponent();
		check(AbilitySystemComponent);

		// For AI characters who don't have PlayerController. For others initting twice with no harm
		AbilitySystemComponent->InitAbilityActorInfo(PS, this);

		CombatAttributes = PS->GetCombatAttributeSet();
		InitializeAttributes();

		// Set Health/Shield to their max. This is only necessary for *Respawn*.
		SetHealth(GetMaxHealth());
		SetShield(GetMaxShield());

		AddCharacterAbilities();
		AddDeathAbility();
		AASPlayerController* PC = Cast<AASPlayerController>(GetController());
		if (PC)
		{
			// Setting asc in pc for tagged inputs
			PC->SetAbilitySystemComponent(AbilitySystemComponent);
		}
		
		EquipmentComponent->InitializeWithAbilitySystem(AbilitySystemComponent);

		if (UASAnimInstance* AnimInst = Cast<UASAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			AnimInst->InitializeWithAbilitySystem(AbilitySystemComponent);
		}
		if (Mesh1P)
		{
			if (UASAnimInstance* AnimInst1P = Cast<UASAnimInstance>(GetHolderMesh1P()->GetAnimInstance()))
			{
				AnimInst1P->InitializeWithAbilitySystem(AbilitySystemComponent);
			}
		}
		if (USkeletalMeshComponent* Mesh3P = GetMesh(); Mesh3P && !IsLocallyControlled())
		{
			Mesh3P->TickPose(FMath::Max(GetWorld()->GetDeltaSeconds(), UE_KINDA_SMALL_NUMBER), false);
		}
	}
}

void AASCharacter::UnPossessed()
{
	RemoveCharacterAbilities();
	Super::UnPossessed();
}

void AASCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	AASPlayerState* PS = GetPlayerState<AASPlayerState>();
	if (PS)
	{
		AbilitySystemComponent = PS->GetASAbilityComponent();

		AbilitySystemComponent->InitAbilityActorInfo(PS, this);

		CombatAttributes = PS->GetCombatAttributeSet();
		
		// If we handle players disconnecting and rejoining in the future, we'll have to change this so that posession from rejoining doesn't reset attributes.
		// For now assume possession = spawn/respawn.
		InitializeAttributes();

		AASPlayerController* PC = Cast<AASPlayerController>(GetController());
		if (PC)
		{
			// Setting asc in pc for tagged inputs
			PC->SetAbilitySystemComponent(AbilitySystemComponent);
		}

		// Set Health/Shield to their max. This is only necessary for *Respawn*.
		SetHealth(GetMaxHealth());
		SetShield(GetMaxShield());
		
		EquipmentComponent->InitializeWithAbilitySystem(AbilitySystemComponent);

		if (UASAnimInstance* AnimInst = Cast<UASAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			AnimInst->InitializeWithAbilitySystem(AbilitySystemComponent);
		}
		if (Mesh1P)
		{
			if (UASAnimInstance* AnimInst1P = Cast<UASAnimInstance>(GetHolderMesh1P()->GetAnimInstance()))
			{
				AnimInst1P->InitializeWithAbilitySystem(AbilitySystemComponent);
			}
		}
	}
}

void AASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AASCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	LinkAnimLayers(nullptr, nullptr);
	if (GetMesh() && GetCharacterMovement())
	{
		GetMesh()->AddTickPrerequisiteComponent(GetCharacterMovement());
	}
}

void AASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	const APlayerController* PC = GetController<APlayerController>();
	const ULocalPlayer* LP = PC->GetLocalPlayer();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	Subsystem->AddMappingContext(DefaultMappingContext, 0);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AASCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AASCharacter::LookUp);
		EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Triggered, this, &AASCharacter::Turn);
	}
}

void AASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AASCharacter, DeathState);
}

UAbilitySystemComponent* AASCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

int32 AASCharacter::GetAbilityLevel() const
{
	return 1;
}

bool AASCharacter::IsAlive() const
{
	return GetHealth() > 0.0f;
}

float AASCharacter::GetHealth() const
{
	if (CombatAttributes)
	{
		return CombatAttributes->GetHealth();
	}
	
	return 0.0f;
}

float AASCharacter::GetMaxHealth() const
{
	if (CombatAttributes)
	{
		return CombatAttributes->GetMaxHealth();
	}
	
	return 0.0f;
}

float AASCharacter::GetShield() const
{
	if (CombatAttributes)
	{
		return CombatAttributes->GetShield();
	}
	
	return 0.0f;
}

float AASCharacter::GetMaxShield() const
{
	if (CombatAttributes)
	{
		return CombatAttributes->GetMaxShield();
	}
	
	return 0.0f;
}

void AASCharacter::RemoveCharacterAbilities()
{
	if (GetLocalRole() < ROLE_Authority || !IsValid(AbilitySystemComponent) || !AbilitySystemComponent->bCharacterAbilitiesGiven)
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.SourceObject == this && CharacterAbilities.Contains(Spec.Ability->GetClass()))
		{
			AbilitiesToRemove.Add(Spec.Handle);
		}
	}

	for (int32 i = 0; i < AbilitiesToRemove.Num(); i++)
	{
		AbilitySystemComponent->ClearAbility(AbilitiesToRemove[i]);
	}

	AbilitySystemComponent->bCharacterAbilitiesGiven = false;
}

void AASCharacter::AddCharacterAbilities()
{
	// Grant abilities only on the server
	if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent || AbilitySystemComponent->bCharacterAbilitiesGiven)
	{
		return;
	}

	for (TSubclassOf<UASGameplayAbility>& StartupAbility : CharacterAbilities)
	{
		if (StartupAbility)
		{
			FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(StartupAbility, 1);
			AbilitySpec.SourceObject = this;
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(StartupAbility.GetDefaultObject()->InputTag);
			AbilitySystemComponent->GiveAbility(AbilitySpec);
		}
	}
	AbilitySystemComponent->bCharacterAbilitiesGiven = true;
}

void AASCharacter::AddDeathAbility()
{
	// Granted once per ASC
	if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent || AbilitySystemComponent->bDeathAbilityGiven || !DeathAbility)
	{
		return;
	}

	FGameplayAbilitySpec AbilitySpec(DeathAbility, 1);
	AbilitySystemComponent->GiveAbility(AbilitySpec);
	AbilitySystemComponent->bDeathAbilityGiven = true;
}

void AASCharacter::InitializeAttributes()
{
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogAS, Warning, TEXT("No AbilitySystemComponent!"));
		return;
	}

	if (!DefaultAttributes)
	{
		UE_LOG(LogAS, Warning, TEXT("No DefaultAttributes effect!"));
		return;
	}

	//can run on server and client
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle EffectSpec = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributes, GetAbilityLevel(), EffectContext);
	if (EffectSpec.IsValid())
	{
		FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*EffectSpec.Data.Get(), AbilitySystemComponent); 
	}
}

void AASCharacter::SetHealth(float Health)
{
	if (CombatAttributes)
	{
		CombatAttributes->SetHealth(Health);
	}
}

void AASCharacter::SetShield(float Health)
{
	if (CombatAttributes)
	{
		CombatAttributes->SetShield(Health);
	}
}

// Server-only
void AASCharacter::StartDeath(const FVector& ImpulseDir, const FVector& ImpulseLocation, FName ImpulseBone)
{
	if (DeathState.bIsDead)
	{
		return;
	}
	
	DeathState.bIsDead = true;
	DeathState.ImpulseDir = ImpulseDir;
	DeathState.ImpulseLocation = ImpulseLocation;
	DeathState.ImpulseBone = ImpulseBone;

	RemoveCharacterAbilities();
	
	if (AController* C = GetController())
	{
		C->SetIgnoreMoveInput(true);
	}

	// Server-side ragdoll so simulated proxies and the authoritative sim stay in sync.
	SetRagdollPhysics();
}

// Server and on all clients
void AASCharacter::SetRagdollPhysics()
{
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	if (Mesh1P)
	{
		Mesh1P->SetVisibility(false);
	}

	USkeletalMeshComponent* Mesh3P = GetMesh();
	if (Mesh3P && Mesh3P->GetPhysicsAsset())
	{
		Mesh3P->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh3P->SetCollisionProfileName(FName(TEXT("DeadBody")));
		Mesh3P->SetAllBodiesSimulatePhysics(true);
		Mesh3P->SetSimulatePhysics(true);
		Mesh3P->WakeAllRigidBodies();
		Mesh3P->bBlendPhysics = true;

		ApplyDeathImpulse();
	}
	else if (Mesh3P)
	{
		// No physics asset to ragdoll with: just hide the body as a fallback.
		Mesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh3P->SetVisibility(false);
	}
}

void AASCharacter::ApplyDeathImpulse()
{
	USkeletalMeshComponent* Mesh3P = GetMesh();
	if (!Mesh3P || !Mesh3P->IsSimulatingPhysics() || DeathState.ImpulseDir.IsNearlyZero())
	{
		return;
	}

	const FVector Impulse = DeathState.ImpulseDir * DeathImpulseStrength;
	if (DeathState.ImpulseBone != NAME_None)
	{
		Mesh3P->AddImpulseAtLocation(Impulse, DeathState.ImpulseLocation, DeathState.ImpulseBone);
	}
	else
	{
		Mesh3P->AddImpulse(Impulse, NAME_None, false);
	}
}

void AASCharacter::OnRep_DeathState()
{
	if (DeathState.bIsDead)
	{
		SetRagdollPhysics();
	}
}

USkeletalMeshComponent* AASCharacter::GetHolderMesh1P() const
{
	return Mesh1P;
}

USkeletalMeshComponent* AASCharacter::GetHolderMesh3P() const
{
	return GetMesh();
}

FName AASCharacter::GetWeapon1PAttachPoint() const
{
	return WeaponAttachSocket1P;
}
FName AASCharacter::GetWeapon3PAttachPoint() const
{
	return WeaponAttachSocket3P;
}

TArray<FName> AASCharacter::Get1PSockets() const
{
	TArray<FName> Out;
	if (const USkeletalMeshComponent* mesh1P = GetHolderMesh1P())
	{
		Out = mesh1P->GetAllSocketNames(); // sockets on the 1P mesh
	}
	return Out;
}

TArray<FName> AASCharacter::Get3PSockets() const
{
	TArray<FName> Out;
	if (const USkeletalMeshComponent* mesh3P = GetMesh())
	{
		Out = mesh3P->GetAllSocketNames(); // sockets on the 3P mesh
	}
	return Out;
}

void AASCharacter::Move(const FInputActionValue& Value)
{
	if (!FMath::IsNearlyZero(Value.GetMagnitude()))
	{
		// add movement in that direction
		FVector MovementVector = Value.Get<FVector>();
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}

}

void AASCharacter::LookUp(const FInputActionValue& Value)//bool bIsPure, float Rate)
{
	float Rate = Value.GetMagnitude();
	if (const UASGameUserSettings* Settings = UASGameUserSettings::GetASGameUserSettings())
	{
		Rate *= Settings->GetLookSensitivity();
	}
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate);
}


void AASCharacter::Turn(const FInputActionValue& Value)
{
	float Rate = Value.GetMagnitude();
	if (const UASGameUserSettings* Settings = UASGameUserSettings::GetASGameUserSettings())
	{
		Rate *= Settings->GetLookSensitivity();
	}
	AddControllerYawInput(Rate);
}

void AASCharacter::LinkAnimLayers(TSubclassOf<UAnimInstance> FPLayer, TSubclassOf<UAnimInstance> TPLayer)
{
	SwapLayer(GetHolderMesh1P(), CurrentFPLayer, FPLayer ? FPLayer : UnarmedFPLayer);
	SwapLayer(GetMesh(),   CurrentTPLayer, TPLayer ? TPLayer : UnarmedTPLayer);
}

void AASCharacter::SwapLayer(USkeletalMeshComponent* Mesh, TSubclassOf<UAnimInstance>& Current, TSubclassOf<UAnimInstance> Wanted)
{
	if (!Mesh || !Wanted || Current == Wanted)
	{
		return;
	}

	// Never unlink first: linking is an atomic overlay, while unlinking drops the node to its
	// self layer, which evaluates as the ref pose.
	Mesh->LinkAnimClassLayers(Wanted);
	Current = Wanted;
}

