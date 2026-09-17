// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASPickup.generated.h"

class UAbilitySystemComponent;
class USphereComponent;
class USoundBase;

UCLASS(Abstract)
class ARENASHOOTER_API AASPickup : public AActor
{
	GENERATED_BODY()
	
public:	
	AASPickup();

	void ResetToActive();
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	// === Per-pickup policy (server only) ===
	virtual bool CanGiveTo(UAbilitySystemComponent* ASC);
	virtual bool GiveTo(UAbilitySystemComponent* ASC) PURE_VIRTUAL(AASPickup::GiveTo, return false;)

	void TryPickup(AActor* Actor);
	void Deactivate();
	void Respawn();

	UFUNCTION()
	void OnRep_IsActive();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayPickupSound();
	void ApplyActiveState();
	
	virtual void OnActiveStateChanged(bool bPlayerTransitionFX){}

	UPROPERTY(EditDefaultsOnly, Category="ArenaShooter|Pickup")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(EditDefaultsOnly, Category="ArenaShooter|Pickup")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(EditDefaultsOnly, Category="ArenaShooter|Pickup")
	TObjectPtr<USoundBase> PickupSound;

	UPROPERTY(EditAnywhere, Category="ArenaShooter|Pickup")
	bool bRespawns = true;

	UPROPERTY(EditAnywhere, Category="ArenaShooter|Pickup", meta = (EditCondition = "bRespawns"))
	float RespawnTime = 15.f;

	UPROPERTY(ReplicatedUsing = OnRep_IsActive)
	bool bIsActive = true;
	
	bool bHasAppliedInitialState = false;

	FTimerHandle RespawnTimer;
};
