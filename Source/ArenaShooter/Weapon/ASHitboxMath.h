#pragma once
#include "CoreMinimal.h"

struct FASWorldCapsule
{
	FVector A;
	FVector B;
	float Radius;
	
	FASWorldCapsule()
		: A(ForceInit)
		, B(ForceInit)
		, Radius(0.0f)
	{}
};

struct FASCapsuleHit
{
	FVector::FReal Distance = -1.f;
	int32 Capsule = INDEX_NONE;
};

/** One character in a snapshot: a sphere bounding its capsules, which sit at [FirstCapsule, FirstCapsule + NumCapsules). */
struct FASHitboxGroup
{
	FVector Center = FVector::ZeroVector;
	float Radius = 0.f;
	int32 FirstCapsule = 0;
	int32 NumCapsules = 0;
};

namespace ASHitboxMath
{
	/**
	 * Distance along the ray to where it first enters the capsule, or a negative value on a miss.
	 * Dir must be unit length. A ray that starts inside the capsule hits at distance 0.
	 */
	ARENASHOOTER_API FVector::FReal RayCapsule(const FVector& Origin, const FVector& Dir, const FASWorldCapsule& Capsule);
	
	ARENASHOOTER_API FASCapsuleHit NearestCapsuleHit(const FVector& Origin, const FVector& Dir, FVector::FReal MaxDistance, TArrayView<const FASWorldCapsule> Capsules);
	
	/** Distance to where the ray enters the sphere: 0 if it starts inside, negative on a miss. Dir must be unit length. */
	ARENASHOOTER_API FVector::FReal RaySphere(const FVector& Origin, const FVector& Dir, const FVector& Center, FVector::FReal Radius);

	/** The bounding sphere of Capsules[FirstCapsule, FirstCapsule + NumCapsules). */
	ARENASHOOTER_API FASHitboxGroup MakeGroup(TArrayView<const FASWorldCapsule> Capsules, int32 FirstCapsule, int32 NumCapsules);

	/**
	 * Like NearestCapsuleHit, but tests a group's capsules only when the ray enters its sphere before
	 * the best hit so far. Pure: safe to call from any thread. The returned Capsule indexes Capsules.
	 */
	ARENASHOOTER_API FASCapsuleHit NearestGroupHit(const FVector& Origin, const FVector& Dir, FVector::FReal MaxDistance,
		TArrayView<const FASHitboxGroup> Groups, TArrayView<const FASWorldCapsule> Capsules);
}
