#include "ASPlayerController.h"
#include "ASPlayerState.h"
#include "AbilitySystem/ASAbilitySystemComponent.h"
#include "Input/ASInputComponent.h"
#include "UI/ASGameHUDController.h"
#include "UI/ASNumberPopRelay.h"
#include "UI/ASNumberPopSubsystem.h"

AASPlayerController::AASPlayerController()
{
	PredictionLatencyReduction = 20.0f;
	ClientBiasPct = 0.5f;
	MaxPredictionPing = 120.f;
	FakeProjectileIdCounter = 1;
}

void AASPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AASPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (!IsLocalController()) return;
	
	if (UASInputComponent* IC = Cast<UASInputComponent>(InputComponent))
	{
		IC->BindAbilityActions(AbilityInputConfig, this, &ThisClass::AbilityInputPressed, &ThisClass::AbilityInputReleased);

		if (OpenGameMenuAction)
		{
			IC->BindAction(OpenGameMenuAction, ETriggerEvent::Started, this, &ThisClass::HandleOpenGameMenu);	
		}
	}
}

void AASPlayerController::SetAbilitySystemComponent(UASAbilitySystemComponent* ASC)
{
	AbilitySystemComponent = ASC;
}

float AASPlayerController::GetForwardPredictionTime() const
{
	return (PlayerState && (GetNetMode() != NM_Standalone)) ?
	(0.001f * ClientBiasPct * FMath::Clamp(PlayerState->ExactPing - (IsLocalController() ? 0.0f : PredictionLatencyReduction), 0.0f, MaxPredictionPing)) : 0.0f;
}

float AASPlayerController::GetProjectileSleepTime() const
{
	// At high latencies, projectiles won't be spawned until they can be forward-predicted at the maximum prediction ping.
	return 0.001f * FMath::Max(0.0f, PlayerState->ExactPing - PredictionLatencyReduction - MaxPredictionPing);
}

uint32 AASPlayerController::GenerateNewFakeProjectileID()
{
	const uint32 NextId = FakeProjectileIdCounter;
	FakeProjectileIdCounter = FakeProjectileIdCounter < UINT32_MAX ? FakeProjectileIdCounter + 1 : 1;
	checkf(!FakeProjectiles.Contains(NextId), TEXT("Generated invalid projectile ID! ID (%i) already used. Fake projectile map size: (%i)."), NextId, FakeProjectiles.Num());
	return NextId;
}

void AASPlayerController::ClientNumberPops_Implementation(const TArray<FASNumberPop>& Pops)
{
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UASNumberPopSubsystem* Subsystem = LP->GetSubsystem<UASNumberPopSubsystem>())
		{
			for (const FASNumberPop& Pop : Pops)
			{
				Subsystem->AddNumberPop(Pop);
			}
		}
	}
}

void AASPlayerController::HandleOpenGameMenu()
{
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UASGameHUDController* UI = LP->GetSubsystem<UASGameHUDController>())
		{
			UI->OpenGameMenu();
		}
	}
}

void AASPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	AASPlayerState* PS = GetPlayerState<AASPlayerState>();
	if (PS)
	{
		// Init ASC with PS (Owner) and our new Pawn (AvatarActor)
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, InPawn);
	}
}

void AASPlayerController::AbilityInputPressed(FGameplayTag InputTag)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityInputPressed(InputTag);
	}
}

void AASPlayerController::AbilityInputReleased(FGameplayTag InputTag)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityInputReleased(InputTag);
	}
}

