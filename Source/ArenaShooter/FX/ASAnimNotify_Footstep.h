// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ASAnimNotify_Footstep.generated.h"

class UASSurfaceSoundSet;

UCLASS()
class ARENASHOOTER_API UASAnimNotify_Footstep : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep")
	FName FootBoneName = "foot_l";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep")
	TObjectPtr<UASSurfaceSoundSet> SoundSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep", meta = (Units = "Centimeters"))
	float TraceUp = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep", meta = (Units = "Centimeters"))
	float TraceDown = 60.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep", meta = (ClampMin = 0, ClampMax = 1))
	float LocalPlayerVolume = 0.5f;
};
