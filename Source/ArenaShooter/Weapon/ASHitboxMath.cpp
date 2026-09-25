#include "ASHitboxMath.h"

FVector::FReal ASHitboxMath::RayCapsule(const FVector& Origin, const FVector& Dir, const FASWorldCapsule& Capsule)
{
	const FVector::FReal Radius = Capsule.Radius;
	if (FMath::PointDistToSegmentSquared(Origin, Capsule.A, Capsule.B) <= Radius * Radius)
	{
		return 0.f;
	}
	
	FVector::FReal Best = -1.f;
	auto Consider = [&Best](FVector::FReal Distance)
	{
		if (Distance >= 0.f && (Best < 0.f || Distance < Best))
		{
			Best = Distance;
		}	
	};
	
	const FVector Axis = Capsule.B - Capsule.A;
	const FVector ToOrigin = Origin - Capsule.A;
	const FVector::FReal AxisSq = Axis.SizeSquared();
	const FVector::FReal AxisDir = Axis | Dir;
	const FVector::FReal AxisToOrigin = Axis | ToOrigin;
	
	const FVector::FReal QA = AxisSq - AxisDir * AxisDir;
	if (QA > KINDA_SMALL_NUMBER)
	{
		const FVector::FReal QB = AxisSq * (ToOrigin | Dir) - AxisToOrigin * AxisDir;
		const FVector::FReal QC = AxisSq * ToOrigin.SizeSquared() - AxisToOrigin * AxisToOrigin - Radius * Radius * AxisSq;
		const FVector::FReal H = QB * QB - QA * QC;
		if (H < 0.f)
		{
			return -1.f; // misses the infinite cylinder, so it misses the capsule inside it too
		}
		
		const FVector::FReal Distance = (-QB - FMath::Sqrt(H)) / QA;
		const FVector::FReal AlongAxis = AxisToOrigin + Distance * AxisDir;
		if (AlongAxis > 0.f && AlongAxis < AxisSq)
		{
			Consider(Distance);
		}
	}
	
	Consider(RaySphere(Origin, Dir, Capsule.A, Radius));
	Consider(RaySphere(Origin, Dir, Capsule.B, Radius));
	
	return Best;
}

FASCapsuleHit ASHitboxMath::NearestCapsuleHit(const FVector& Origin, const FVector& Dir, FVector::FReal MaxDistance, TArrayView<const FASWorldCapsule> Capsules)
{
	FASCapsuleHit Nearest;
	FVector::FReal Limit = MaxDistance;
	for (int32 Index = 0; Index < Capsules.Num(); ++Index)
	{
		const FVector::FReal Distance = RayCapsule(Origin, Dir, Capsules[Index]);
		if (Distance >= 0. && Distance < Limit)
		{
			Limit = Distance;
			Nearest.Distance = Distance;
			Nearest.Capsule = Index;
		}
	}
	return Nearest;
}

FVector::FReal ASHitboxMath::RaySphere(const FVector& Origin, const FVector& Dir, const FVector& Center, FVector::FReal Radius)
{
	const FVector ToOrigin = Origin - Center;
	const FVector::FReal  C = ToOrigin.SizeSquared() - Radius * Radius;
	if (C <= 0.)
	{
		return 0.; // starts inside
	}

	const FVector::FReal  B = ToOrigin | Dir;
	if (B > 0.)
	{
		return -1.; // outside and moving away
	}

	const FVector::FReal  H = B * B - C;
	return H < 0. ? -1. : -B - FMath::Sqrt(H);
}

FASHitboxGroup ASHitboxMath::MakeGroup(TArrayView<const FASWorldCapsule> Capsules, int32 FirstCapsule, int32 NumCapsules)
{
	FASHitboxGroup Group;
	Group.FirstCapsule = FirstCapsule;
	Group.NumCapsules = NumCapsules;

	const TArrayView<const FASWorldCapsule> Members = Capsules.Slice(FirstCapsule, NumCapsules);
	FBox Box(ForceInit);
	for (const FASWorldCapsule& Capsule : Members)
	{
		Box += Capsule.A;
		Box += Capsule.B;
	}
	Group.Center = Box.GetCenter();

	// A capsule lies within Radius of its segment, and the segment within reach of its farther endpoint.
	for (const FASWorldCapsule& Capsule : Members)
	{
		const FVector::FReal Reach = FMath::Max(FVector::Dist(Group.Center, Capsule.A), FVector::Dist(Group.Center, Capsule.B)) + Capsule.Radius;
		Group.Radius = FMath::Max(Group.Radius, static_cast<float>(Reach));
	}
	return Group;
}

FASCapsuleHit ASHitboxMath::NearestGroupHit(const FVector& Origin, const FVector& Dir, FVector::FReal MaxDistance, TArrayView<const FASHitboxGroup> Groups, TArrayView<const FASWorldCapsule> Capsules)
{
	FASCapsuleHit Nearest;
	FVector::FReal Limit = MaxDistance;
	for (const FASHitboxGroup& Group : Groups)
	{
		const FVector::FReal Entry = RaySphere(Origin, Dir, Group.Center, Group.Radius);
		if (Entry < 0. || Entry >= Limit)
		{
			continue;
		}

		const FASCapsuleHit GroupHit = NearestCapsuleHit(Origin, Dir, Limit, Capsules.Slice(Group.FirstCapsule, Group.NumCapsules));
		if (GroupHit.Capsule != INDEX_NONE)
		{
			Limit = GroupHit.Distance;
			Nearest.Distance = GroupHit.Distance;
			Nearest.Capsule = Group.FirstCapsule + GroupHit.Capsule;
		}
	}
	return Nearest;
}
