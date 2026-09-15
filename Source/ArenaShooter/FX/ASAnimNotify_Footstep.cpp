// Fill out your copyright notice in the Description page of Project Settings.


#include "ASAnimNotify_Footstep.h"

#include "ASSurfaceSoundSet.h"
#include "Kismet/GameplayStatics.h"

void UASAnimNotify_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	UWorld* World = MeshComp ? MeshComp->GetWorld() : nullptr;
	if (!World || !SoundSet || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	
	const FVector FootLocation = MeshComp->GetSocketLocation(FootBoneName);
	
	if (World->WorldType == EWorldType::EditorPreview)
	{
		UGameplayStatics::PlaySoundAtLocation(MeshComp, SoundSet->DefaultSound, FootLocation);
		return;
	}
	
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FootStepTrace), false, MeshComp->GetOwner());
	Params.bReturnPhysicalMaterial = true;
	
	FHitResult Hit;
	
	const FVector Start = FootLocation + FVector(0., 0., TraceUp);
	const FVector End = FootLocation - FVector(0., 0., TraceDown);
	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return;
	}

	USoundBase* Sound = SoundSet->GetSound(UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()));
	if (!Sound)
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(MeshComp->GetOwner());
	const float Volume = Pawn && Pawn->IsLocallyControlled() ? LocalPlayerVolume : 1.f;

	UGameplayStatics::PlaySoundAtLocation(MeshComp, Sound, Hit.ImpactPoint, FRotator::ZeroRotator,
		Volume, 1.f, 0.f, nullptr, nullptr, MeshComp->GetOwner());
}
