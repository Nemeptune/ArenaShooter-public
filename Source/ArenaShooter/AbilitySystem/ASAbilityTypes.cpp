#include "ASAbilityTypes.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Audio/AudioDebug.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"

FProjectileDecalInfo::FProjectileDecalInfo()
{
	DecalMaterial = nullptr;
	DecalSize = FVector(128.f, 256.f, 256.f);
	FadeOutStartDelay = 2.f;
	FadeOutDuration = 0.f;
}

void FProjectileDecalInfo::SpawnDecal(const UObject* WorldContextObject, const FVector& Location, const FRotator& Rotation, USceneComponent* AttachToComponent)
{
	if (!DecalMaterial)
	{
		return;
	}

	UDecalComponent* SpawnedDecal;

	if (AttachToComponent)
	{
		SpawnedDecal = UGameplayStatics::SpawnDecalAttached(DecalMaterial, DecalSize, AttachToComponent, NAME_None, Location, Rotation, EAttachLocation::KeepWorldPosition);
	}
	else
	{
		SpawnedDecal = UGameplayStatics::SpawnDecalAtLocation(WorldContextObject, DecalMaterial, DecalSize, Location, Rotation);
	}

	if (SpawnedDecal)
	{
		SpawnedDecal->SetFadeOut(FadeOutStartDelay, FadeOutDuration, false);
	}
}

void FSpawnedProjectileFX::StopEffects()
{
	for (UNiagaraComponent* Particle : SpawnedParticles)
	{
		if (IsValid(Particle))
		{
			if (Cast<UNiagaraSystem>(Particle->GetFXSystemAsset())->IsLooping())
			{
				Particle->SetForceSolo(true);
				Particle->TickComponent(0.0f, LEVELTICK_All, nullptr);
				Particle->SetForceSolo(false);

				Particle->Deactivate();
				Particle->SetAutoDestroy(true);
			}
		}
	}

	for (UAudioComponent* Sound : SpawnedSounds)
	{
		if (IsValid(Sound))
		{
			if (Sound->GetSound() && !Sound->GetSound()->IsOneShot())
			{
				Sound->Stop();
			}
		}
	}

	SpawnedParticles.Empty();
	SpawnedSounds.Empty();
}

void FProjectileFX::ExecuteEffects(const UObject* WorldContextObject, const FVector& Location, const FRotator& Rotation, USceneComponent* AttachComponent, bool bSkipDecal)
{
	if (Particles != nullptr)
	{
		if (UNiagaraComponent* SpawnedSystem = UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContextObject,
			Particles, Location, Rotation, FVector(1.0f), true, true, ENCPoolMethod::AutoRelease))
		{
			SpawnedSystem->SetCastShadow(false);
			SpawnedFX.SpawnedParticles.Add(SpawnedSystem);
		}
	}

	if (Sound != nullptr)
	{
		if (UAudioComponent* SpawnedSound = UGameplayStatics::SpawnSoundAtLocation(WorldContextObject, Sound, Location, Rotation))
		{
			SpawnedFX.SpawnedSounds.Add(SpawnedSound);
		}
	}

	if (!bSkipDecal)
	{
		Decal.SpawnDecal(WorldContextObject, Location, Rotation * -1.0f, AttachComponent);
	}
}
