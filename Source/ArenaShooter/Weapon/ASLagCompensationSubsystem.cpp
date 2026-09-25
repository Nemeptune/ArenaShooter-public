// Fill out your copyright notice in the Description page of Project Settings.


#include "ASLagCompensationSubsystem.h"

#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "System/ASLogChannels.h"
#include "System/ASProfiling.h"
#include "Async/ParallelFor.h"

static TAutoConsoleVariable<bool> CVarLagCompEnabled(TEXT("AS.LagComp.Enabled"), true,
	TEXT("Trace shots against characters where the shooter saw them. 0 traces their current poses."), ECVF_Cheat);

static TAutoConsoleVariable<float> CVarLagCompMaxRewindMs(TEXT("AS.LagComp.MaxRewindMs"), 250.f,
	TEXT("Longest rewind. Shooters with a higher ping have to lead their targets by the rest."), ECVF_Cheat);

static TAutoConsoleVariable<float> CVarLagCompExtraRewindMs(TEXT("AS.LagComp.ExtraRewindMs"), 0.f,
	TEXT("Added to every rewind, on top of the moment the client reports."), ECVF_Cheat);

static TAutoConsoleVariable<bool> CVarLagCompDebug(TEXT("AS.LagComp.Debug"), false,
	TEXT("Draw the rewound capsules of every trace in the server's world."), ECVF_Cheat);

static TAutoConsoleVariable<bool> CVarLagCompParallel(TEXT("AS.LagComp.Parallel"),true,
	TEXT("Spread the rays of one trace over worker threads. 0 runs them all on the game thread."), ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarLagCompMinRaysPerTask(TEXT("AS.LagComp.MinRaysPerTask"),8,
	TEXT("Fewest rays per worker task. Smaller batches run on the game thread, where scheduling would cost more than it saves."), ECVF_Cheat);

void FASTrackedCharacter::AppendCapsulesAt(double Time, TArray<FASWorldCapsule>& OutCapsules) const
{
	if (Frames.IsEmpty())
	{
		return;
	}

	// Walk back from the newest frame to the first one at or before Time, and blend it with the one after.
	const int32 NumFrames = Frames.Num();
	const FASHitboxFrame* Newer = &Frames[NewestFrame];
	for (int32 Step = 1; Step < NumFrames && Newer->Time > Time; ++Step)
	{
		const FASHitboxFrame& Older = Frames[(NewestFrame - Step + NumFrames) % NumFrames];
		if (Older.Time <= Time)
		{
			const float Alpha = static_cast<float>((Time - Older.Time) / (Newer->Time - Older.Time));
			for (int32 Index = 0; Index < Older.Capsules.Num(); ++Index)
			{
				FASWorldCapsule& Capsule = OutCapsules.AddDefaulted_GetRef();
				Capsule.A = FMath::Lerp(Older.Capsules[Index].A, Newer->Capsules[Index].A, Alpha);
				Capsule.B = FMath::Lerp(Older.Capsules[Index].B, Newer->Capsules[Index].B, Alpha);
				Capsule.Radius = Newer->Capsules[Index].Radius;
			}
			return;
		}
		Newer = &Older;
	}

	// At or after the newest frame, or before the oldest: the nearest frame as it is.
	OutCapsules.Append(Newer->Capsules);
}

void UASLagCompensationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PostActorTickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &UASLagCompensationSubsystem::CaptureFrame);
}

void UASLagCompensationSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldPostActorTick.Remove(PostActorTickHandle);
	TrackedCharacters.Empty();
	Super::Deinitialize();
}

bool UASLagCompensationSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UASLagCompensationSubsystem::RegisterCharacter(ACharacter* Character)
{
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	TArray<FASHitCapsuleDef> CapsuleDefs;
	if (!Mesh || !BuildCapsuleDefs(*Mesh, CapsuleDefs))
	{
		UE_LOG(LogAS_Weapon, Warning, TEXT("Lag compensation: %s has no physics asset, so shots trace its current pose"), *GetNameSafe(Character));
		return;
	}

	FASTrackedCharacter& Tracked = TrackedCharacters.AddDefaulted_GetRef();
	Tracked.Character = Character;
	Tracked.Mesh = Mesh;
	Tracked.CapsuleDefs = MoveTemp(CapsuleDefs);
}

void UASLagCompensationSubsystem::UnregisterCharacter(const ACharacter* Character)
{
	TrackedCharacters.RemoveAllSwap([Character](const FASTrackedCharacter& Tracked)
	{
		return !Tracked.Character.IsValid() || Tracked.Character.Get() == Character;
	});
}

void UASLagCompensationSubsystem::CaptureFrame(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	if (World != GetWorld())
	{
		return;
	}
	
	ShownServerTime = GetClientViewServerTime();

	if (TrackedCharacters.IsEmpty())
	{
		return;
	}
	TRACE_CPUPROFILER_EVENT_SCOPE(UASLagCompensationSubsystem::CaptureFrame);
	
	const double Now = World->GetTimeSeconds();
	// A little more than the longest rewind, so a maximum rewind still has two frames to blend.
	const double KeepAfter = Now - CVarLagCompMaxRewindMs.GetValueOnGameThread() * 0.001 - 0.1;
	
	for (FASTrackedCharacter& Tracked : TrackedCharacters)
	{
		const USkeletalMeshComponent* Mesh = Tracked.Mesh.Get();
		if (!Mesh || (Tracked.NewestFrame != INDEX_NONE && Tracked.Frames[Tracked.NewestFrame].Time >= Now))
		{
			continue;
		}
		
		int32 Slot = Tracked.Frames.IsEmpty() ? 0 : (Tracked.NewestFrame + 1) % Tracked.Frames.Num();
		if (Tracked.Frames.IsEmpty() || Tracked.Frames[Slot].Time >= KeepAfter)
		{
			// The oldest frame is still needed: grow the ring of overwriting it.
			Slot = Tracked.NewestFrame + 1;
			Tracked.Frames.InsertDefaulted(Slot);
		}
		Tracked.NewestFrame = Slot;
		
		FASHitboxFrame& Frame = Tracked.Frames[Slot];
		Frame.Time = Now;
		Frame.Capsules.SetNumUninitialized(Tracked.CapsuleDefs.Num());
		PoseCapsules(*Mesh, Tracked.CapsuleDefs, Frame.Capsules);
	}
}

void UASLagCompensationSubsystem::TraceRays(const UWorld& World, TArrayView<const FASShotRay> Rays, ECollisionChannel Channel, const FCollisionQueryParams& Params, TArrayView<const FASWorldCapsule> Capsules, TArrayView<const FASHitboxGroup> Groups, int32 MinRaysPerTask, bool bParallel, TArrayView<FHitResult> OutWorldHits, TArrayView<FASCapsuleHit> OutCapsuleHits)
{
	check(Rays.Num() == OutWorldHits.Num() && Rays.Num() == OutCapsuleHits.Num());

	// World traces are read-only scene queries, which the engine's own async traces also run on workers.
	// Dedicated servers run this on one thread unless launched with -useperfthreads.
	ParallelFor(TEXT("LagComp_TraceRays"), Rays.Num(), FMath::Max(MinRaysPerTask, 1), [&](int32 Index)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(LineTraceRewound_Ray);

		const FASShotRay& Ray = Rays[Index];
		FHitResult& WorldHit = OutWorldHits[Index];
		const bool bWorldHit = World.LineTraceSingleByChannel(WorldHit, Ray.Start, Ray.End, Channel, Params);

		const FVector Dir = (Ray.End - Ray.Start).GetSafeNormal();
		const FVector::FReal MaxDistance = bWorldHit ? WorldHit.Distance : FVector::Dist(Ray.Start, Ray.End);
		OutCapsuleHits[Index] = ASHitboxMath::NearestGroupHit(Ray.Start, Dir, MaxDistance, Groups, Capsules);
	}, bParallel ? EParallelForFlags::None : EParallelForFlags::ForceSingleThread);
}

bool UASLagCompensationSubsystem::BuildCapsuleDefs(const USkeletalMeshComponent& Mesh, TArray<FASHitCapsuleDef>& OutDefs)
{
	const UPhysicsAsset* PhysicsAsset = Mesh.GetPhysicsAsset();
	if (!PhysicsAsset)
	{
		return false;
	}

	for (const USkeletalBodySetup* BodySetup : PhysicsAsset->SkeletalBodySetups)
	{
		const int32 BoneIndex = BodySetup ? Mesh.GetBoneIndex(BodySetup->BoneName) : INDEX_NONE;
		if (BoneIndex == INDEX_NONE)
		{
			continue;
		}

		// The material a trace against this body returns, so zones and damage multipliers carry over.
		const FBodyInstance* Body = Mesh.GetBodyInstance(BodySetup->BoneName);
		UPhysicalMaterial* Zone = Body ? Body->GetSimplePhysicalMaterial() : BodySetup->PhysMaterial.Get();

		const FKAggregateGeom& Geometry = BodySetup->AggGeom;
		for (const FKSphylElem& Sphyl : Geometry.SphylElems)
		{
			const FVector HalfSegment = Sphyl.Rotation.RotateVector(FVector(0., 0., Sphyl.Length * 0.5));
			OutDefs.Add({ BoneIndex, BodySetup->BoneName, Sphyl.Center - HalfSegment, Sphyl.Center + HalfSegment, Sphyl.Radius, Zone });
		}
		for (const FKSphereElem& Sphere : Geometry.SphereElems)
		{
			OutDefs.Add({ BoneIndex, BodySetup->BoneName, Sphere.Center, Sphere.Center, Sphere.Radius, Zone });
		}

		UE_CLOG(!Geometry.BoxElems.IsEmpty() || !Geometry.ConvexElems.IsEmpty() || !Geometry.TaperedCapsuleElems.IsEmpty(),
			LogAS_Weapon, Warning, TEXT("Lag compensation: body %s of %s has boxes, convexes or tapered capsules, which rewound traces ignore"),
			*BodySetup->BoneName.ToString(), *GetNameSafe(PhysicsAsset));
	}
	return true;
}

void UASLagCompensationSubsystem::PoseCapsules(const USkeletalMeshComponent& Mesh, TArrayView<const FASHitCapsuleDef> Defs, TArrayView<FASWorldCapsule> OutCapsules)
{
	check(Defs.Num() == OutCapsules.Num());
	for (int32 Index = 0; Index < Defs.Num(); ++Index)
	{
		const FASHitCapsuleDef& Def = Defs[Index];
		const FTransform Bone = Mesh.GetBoneTransform(Def.BoneIndex);

		FASWorldCapsule& Capsule = OutCapsules[Index];
		Capsule.A = Bone.TransformPosition(Def.LocalA);
		Capsule.B = Bone.TransformPosition(Def.LocalB);
		Capsule.Radius = Def.Radius * static_cast<float>(Bone.GetMaximumAxisScale());
	}
}

#if ENABLE_DRAW_DEBUG
static void DrawCapsule(const UWorld* World, const FASWorldCapsule& Capsule, const FColor& Color)
{
	const FVector Axis = Capsule.B - Capsule.A;
	const FQuat Rotation = Axis.IsNearlyZero() ? FQuat::Identity : FRotationMatrix::MakeFromZ(Axis).ToQuat();
	const float HalfHeight = static_cast<float>(Axis.Size() * 0.5) + Capsule.Radius;
	DrawDebugCapsule(World, (Capsule.A + Capsule.B) * 0.5, HalfHeight, Capsule.Radius, Rotation, Color, false, 0.1f);
}
#endif

void UASLagCompensationSubsystem::LineTraceRewound(TArrayView<const FASShotRay> Rays, ECollisionChannel Channel, FCollisionQueryParams Params, const AActor* Shooter, TArrayView<FHitResult> OutHits) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UASLagCompensationSubsystem::LineTraceRewound);
	check(Rays.Num() == OutHits.Num());
	
	UWorld* World = GetWorld();
	
	struct FCapsuleSource
	{
		int32 Tracked;
		int32 Def;
	};
	
	TArray<FASWorldCapsule> Capsules;
	TArray<FCapsuleSource> Sources;
	TArray<FASHitboxGroup> Groups;
	
	double RewindSeconds = 0.f;
	
	if (CVarLagCompEnabled.GetValueOnGameThread())
	{
		const double Time = GetShooterViewTime(Shooter);
		RewindSeconds = World->GetTimeSeconds() - Time;
		CSV_CUSTOM_STAT(ArenaShooter, LagCompRewindMs, static_cast<float>(RewindSeconds * 1000.), ECsvCustomStatOp::Max);

		for (int32 TrackedIndex = 0; TrackedIndex < TrackedCharacters.Num(); ++TrackedIndex)
		{
			const FASTrackedCharacter& Tracked = TrackedCharacters[TrackedIndex];
			const ACharacter* Character = Tracked.Character.Get();
			if (!Character)
			{
				continue;
			}

			Params.AddIgnoredActor(Character); // characters are hit only through their capsules
			if (Character == Shooter)
			{
				continue;
			}

			const int32 First = Capsules.Num();
			Tracked.AppendCapsulesAt(Time, Capsules);
			if (Capsules.Num() > First)
			{
				Groups.Add(ASHitboxMath::MakeGroup(Capsules, First, Capsules.Num() - First));
			}
			for (int32 DefIndex = 0; First + DefIndex < Capsules.Num(); ++DefIndex)
			{
				Sources.Add({ TrackedIndex, DefIndex });
			}
		}
	}

	// Workers: rays are independent and each writes only its own slots, so nothing is locked. World
	// traces are read-only scene queries, which the engine's own async traces also run on workers.
	// Dedicated servers run this on one thread unless launched with -useperfthreads.
	TArray<FASCapsuleHit> CapsuleHits;
	CapsuleHits.SetNum(Rays.Num());
	TraceRays(*World, Rays, Channel, Params, Capsules, Groups,
		CVarLagCompMinRaysPerTask.GetValueOnGameThread(), CVarLagCompParallel.GetValueOnGameThread(), OutHits, CapsuleHits);
	CSV_CUSTOM_STAT(ArenaShooter, LagCompRays, Rays.Num(), ECsvCustomStatOp::Accumulate);

	// Game thread: turn capsule hits into the hit results the damage code reads.
	for (int32 Index = 0; Index < Rays.Num(); ++Index)
	{
		const FASCapsuleHit& CapsuleHit = CapsuleHits[Index];
		if (CapsuleHit.Capsule == INDEX_NONE)
		{
			continue;
		}

		const FCapsuleSource& Source = Sources[CapsuleHit.Capsule];
		const FASTrackedCharacter& Tracked = TrackedCharacters[Source.Tracked];
		const FASHitCapsuleDef& Def = Tracked.CapsuleDefs[Source.Def];
		const FASWorldCapsule& Capsule = Capsules[CapsuleHit.Capsule];
		const FASShotRay& Ray = Rays[Index];

		const FVector Dir = (Ray.End - Ray.Start).GetSafeNormal();
		const FVector ImpactPoint = Ray.Start + Dir * CapsuleHit.Distance;
		FVector ImpactNormal = (ImpactPoint - FMath::ClosestPointOnSegment(ImpactPoint, Capsule.A, Capsule.B)).GetSafeNormal();
		if (ImpactNormal.IsZero())
		{
			ImpactNormal = -Dir; // the ray started inside the capsule
		}

		FHitResult& Hit = OutHits[Index];
		Hit = FHitResult();
		Hit.bBlockingHit = true;
		Hit.HitObjectHandle = FActorInstanceHandle(Tracked.Character.Get());
		Hit.Component = Tracked.Mesh.Get();
		Hit.TraceStart = Ray.Start;
		Hit.TraceEnd = Ray.End;
		Hit.Location = ImpactPoint;
		Hit.ImpactPoint = ImpactPoint;
		Hit.Normal = ImpactNormal;
		Hit.ImpactNormal = ImpactNormal;
		Hit.Distance = CapsuleHit.Distance;
		Hit.Time = static_cast<float>(CapsuleHit.Distance / FVector::Dist(Ray.Start, Ray.End));
		Hit.BoneName = Def.BoneName;
		Hit.PhysMaterial = Def.Zone;

#if ENABLE_DRAW_DEBUG
		if (CVarLagCompDebug.GetValueOnGameThread())
		{
			DrawCapsule(World, Capsule, FColor::Red);
		}
#endif

		UE_LOG(LogAS_Weapon, Verbose, TEXT("Rewound hit: %s %s, rewind %.0f ms"),
			*GetNameSafe(Hit.GetActor()), *Def.BoneName.ToString(), RewindSeconds * 1000.);
	}

#if ENABLE_DRAW_DEBUG
	if (CVarLagCompDebug.GetValueOnGameThread())
	{
		for (const FASWorldCapsule& Capsule : Capsules)
		{
			DrawCapsule(World, Capsule, FColor::Orange);
		}
		for (const FASHitboxGroup& Group : Groups)
		{
			DrawDebugSphere(World, Group.Center, Group.Radius, 12, FColor::Cyan, false, 0.1f);
		}
	}
#endif
}

void UASLagCompensationSubsystem::NoteServerSnapshot(double ServerTime)
{
	if (ServerTime > NewestSnapshotServerTime)
	{
		NewestSnapshotServerTime = ServerTime;
		NewestSnapshotLocalTime = GetWorld()->GetTimeSeconds();
	}
}

double UASLagCompensationSubsystem::GetClientViewServerTime() const
{
	if (NewestSnapshotServerTime <= 0.)
	{
		return 0.;
	}
	return NewestSnapshotServerTime + (GetWorld()->GetTimeSeconds() - NewestSnapshotLocalTime);
}

void UASLagCompensationSubsystem::NoteClientViewTime(const ACharacter* Character, double ViewServerTime)
{
	FASTrackedCharacter* Tracked = TrackedCharacters.FindByPredicate([Character](const FASTrackedCharacter& Entry)
	{
		return Entry.Character.Get() == Character;
	});
	if (Tracked)
	{
		// One packet can hold an old, a pending and a new move: keep the newest moment.
		Tracked->ClientViewTime = FMath::Max(Tracked->ClientViewTime, ViewServerTime);
	}
}

double UASLagCompensationSubsystem::GetShownServerTime() const
{
	return ShownServerTime;
}

double UASLagCompensationSubsystem::GetShooterViewTime(const AActor* Shooter) const
{
	const double Now = GetWorld()->GetTimeSeconds();
	const APawn* Pawn = Cast<APawn>(Shooter);
	if (!Pawn || Pawn->IsLocallyControlled())
	{
		return Now; // a listen-server host sees characters where the server has them
	}

	const FASTrackedCharacter* Tracked = TrackedCharacters.FindByPredicate([Pawn](const FASTrackedCharacter& Entry)
	{
		return Entry.Character.Get() == Pawn;
	});
	if (!Tracked || Tracked->ClientViewTime <= 0.)
	{
		return Now; // no stamped move yet
	}

	// Clamped: a client can't pull the rewind further back than MaxRewindMs, or into the future.
	const double ViewTime = Tracked->ClientViewTime - CVarLagCompExtraRewindMs.GetValueOnGameThread() * 0.001;
	return FMath::Clamp(ViewTime, Now - CVarLagCompMaxRewindMs.GetValueOnGameThread() * 0.001, Now);
}
