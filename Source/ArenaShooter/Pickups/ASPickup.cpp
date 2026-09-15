// Fill out your copyright notice in the Description page of Project Settings.


#include "ASPickup.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "System/ASGameplayTags.h"
#include "GameplayTagContainer.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


AASPickup::AASPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetSphereRadius(80.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(CollisionSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned
void AASPickup::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AASPickup::OnSphereOverlap);
	}
	
	ApplyActiveState();
}

void AASPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AASPickup, bIsActive);
}

void AASPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryPickup(OtherActor);
}

bool AASPickup::CanGiveTo(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return false;
	}

	FGameplayTagContainer DeathTags;
	DeathTags.AddTag(FASGameplayTags::Status_Death_Dead);
	DeathTags.AddTag(FASGameplayTags::Status_Death_Dying);
	return !ASC->HasAnyMatchingGameplayTags(DeathTags);
}

void AASPickup::TryPickup(AActor* Actor)
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);

	if (!bIsActive || !CanGiveTo(ASC))
	{
		return;
	}

	if (GiveTo(Actor,ASC))
	{
		Deactivate();
	}
}

void AASPickup::Deactivate()
{
	bIsActive = false;
	ApplyActiveState();

	Multicast_PlayPickupSound();

	if (bRespawns)
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &AASPickup::Respawn, RespawnTime, false);
	}
}

void AASPickup::Respawn()
{
	bIsActive = true;
	ApplyActiveState();

	// player may already be standing on the pickup place when it comes back
	TArray<AActor*> Overlapping;
	CollisionSphere->GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		TryPickup(Actor);
		if (!bIsActive)
		{
			break;
		}
	}
}

void AASPickup::OnRep_IsActive()
{
	ApplyActiveState();
}

void AASPickup::Multicast_PlayPickupSound_Implementation()
{
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}
}

void AASPickup::ApplyActiveState()
{
	SetActorHiddenInGame(!bIsActive);
	SetActorEnableCollision(bIsActive);
	
	OnActiveStateChanged(bHasAppliedInitialState);
	bHasAppliedInitialState = true;
}
