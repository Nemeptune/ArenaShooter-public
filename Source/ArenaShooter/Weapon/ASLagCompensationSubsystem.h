// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASHitboxMath.h"
#include "Subsystems/WorldSubsystem.h"
#include "ASLagCompensationSubsystem.generated.h"

class ACharacter;
class UPhysicalMaterial;
class USkeletalMeshComponent;

struct FASHitCapsuleDef
{
	int32 BoneIndex;
	FName BoneName;
	FVector LocalA;
	FVector LocalB;
	float Radius;
	TWeakObjectPtr<UPhysicalMaterial> Zone;
	FASHitCapsuleDef()
		: BoneIndex(INDEX_NONE)
		, BoneName("")
		, LocalA(ForceInit)
		, LocalB(ForceInit)
		, Radius(0.f)
		, Zone(nullptr)
	{}
	
	FASHitCapsuleDef(int32 InBoneIndex,
	FName InBoneName,
	FVector InLocalA,
	FVector InLocalB,
	float InRadius,
	TWeakObjectPtr<UPhysicalMaterial> InZone)
	: BoneIndex(InBoneIndex)
	, BoneName(InBoneName)
	, LocalA(InLocalA)
	, LocalB(InLocalB)
	, Radius(InRadius)
	, Zone(InZone)
	{}
};

struct FASHitboxFrame
{
	double Time = 0.f;
	TArray<FASWorldCapsule> Capsules;
};

struct FASShotRay
{
	FVector Start;
	FVector End;
	
	FASShotRay()
		: Start(ForceInit)
		, End(ForceInit)
	{}
	
	FASShotRay(const FVector& InStart,const FVector& InEnd)
		: Start(InStart)
		, End(InEnd)
	{}
};

struct FASTrackedCharacter
{
	TWeakObjectPtr<ACharacter> Character;
	TWeakObjectPtr<USkeletalMeshComponent> Mesh;
	TArray<FASHitCapsuleDef> CapsuleDefs;
	
	// Ring buffer. It grows instead of overwriting while its oldest frame is still inside the history window.
	TArray<FASHitboxFrame> Frames;
	int32 NewestFrame = INDEX_NONE;
	
	// Server time the character's client was seeing when it made its newest move. Zero until the first one.
	double ClientViewTime = 0.;
	
	/** The capsules as they were at Time, blended between the two frames around it. */
	void AppendCapsulesAt(double Time, TArray<FASWorldCapsule>& OutCapsules) const;
};

/**
 * Server-side lag compensation. Records every registered character's hit capsules at the end of
 * each frame, and traces shots against the poses the shooter saw: now minus their round trip.
 * Capsules and zone materials come from the mesh's physics asset, so a rewound hit carries the
 * same data a plain weapon trace would.
 */
UCLASS()
class ARENASHOOTER_API UASLagCompensationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	/** Server. Registered characters are hit only through their recorded capsules. */
	void RegisterCharacter(ACharacter* Character);
	void UnregisterCharacter(const ACharacter* Character);
	
	/**
	 * Line trace against the world as it is now and every registered character as the shooter saw
	 * it. The shooter's own character is skipped. Character hits carry the bone and the zone material.
	 */
	void LineTraceRewound(TArrayView<const FASShotRay> Rays, ECollisionChannel Channel, 
		FCollisionQueryParams Params, const AActor* Shooter, TArrayView<FHitResult> OutHits) const;
	
	/** Client: a simulated character's movement from ServerTime just arrived. */
	void NoteServerSnapshot(double ServerTime);

	/** Server: the character's client made a move while seeing ViewServerTime. */
	void NoteClientViewTime(const ACharacter* Character, double ViewServerTime);
	
	/**
	 * Client: the server time of the last completed frame, which is what the player sees and reacts to
	 * while the next frame builds its move. Zero before the first snapshot.
	 */
	double GetShownServerTime() const;

	/** Server time to trace this shooter's shots at: the moment their client was seeing. */
	double GetShooterViewTime(const AActor* Shooter) const;
	
	static void TraceRays(const UWorld& World, TArrayView<const FASShotRay> Rays, ECollisionChannel Channel, const FCollisionQueryParams& Params,
		TArrayView<const FASWorldCapsule> Capsules, TArrayView<const FASHitboxGroup> Groups, int32 MinRaysPerTask, bool bParallel,
		TArrayView<FHitResult> OutWorldHits, TArrayView<FASCapsuleHit> OutCapsuleHits);
	
	static bool BuildCapsuleDefs(const USkeletalMeshComponent& Mesh, TArray<FASHitCapsuleDef>& OutDefs);
	
	static void PoseCapsules(const USkeletalMeshComponent& Mesh, TArrayView<const FASHitCapsuleDef> Defs, TArrayView<FASWorldCapsule> OutCapsules);
	
protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	
private:
	/**
	 * Client: the server time of the world this machine shows. Simulated characters display their newest
	 * snapshot moved forward since it arrived, so it's that snapshot's time plus the time since. Zero before the first.
	 */
	double GetClientViewServerTime() const;
	
	void CaptureFrame(UWorld* World, ELevelTick TickType, float DeltaSeconds);
	
	TArray<FASTrackedCharacter> TrackedCharacters;
	FDelegateHandle PostActorTickHandle;
	
	double NewestSnapshotServerTime = 0.;
	double NewestSnapshotLocalTime = 0.;
	double ShownServerTime = 0.;
};
