// Fill out your copyright notice in the Description page of Project Settings.


#include "ASGameplayAbility_ProjectileWeapon.h"

#include "ArenaShooter.h"
#include "Components/SphereComponent.h"
#include "Weapon/ASProjectile.h"

void UASGameplayAbility_ProjectileWeapon::GetProjectileFireTransform(FVector& OutLocation, FRotator& OutRotation) const
{
	OutLocation = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;

	if (!GetWeaponViewpoint(OutLocation, OutRotation))
	{
		return;
	}

	// The server discards these for remote clients and waits on replicated target data, so
	// there is nothing to compute there.
	if (!IsLocallyControlled())
	{
		return;
	}
	
	if (FireOffset.IsNearlyZero())
	{
		return;   // firing straight from the eye, nothing to clear
	}

	const FVector ViewLocation = OutLocation;
	const FVector Desired = ViewLocation + OutRotation.RotateVector(FireOffset);

	float Radius = 1.f;
	if (const AASProjectile* CDO = *ProjectileClass ? ProjectileClass.GetDefaultObject() : nullptr)
	{
		if (const USphereComponent* Sphere = Cast<USphereComponent>(CDO->GetRootComponent()))
		{
			Radius = FMath::Max(Sphere->GetUnscaledSphereRadius(), 1.f);
		}
	}

	// Simple collision only. This is a clearance test for the spawn point, not a hit test.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectileFireStart), false,
								 GetAvatarActorFromActorInfo());

	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, ViewLocation, Desired, FQuat::Identity,
			COLLISION_WEAPON, FCollisionShape::MakeSphere(Radius), Params))
	{
		OutLocation = Hit.Location;
		return;
	}

	OutLocation = Desired;
}
