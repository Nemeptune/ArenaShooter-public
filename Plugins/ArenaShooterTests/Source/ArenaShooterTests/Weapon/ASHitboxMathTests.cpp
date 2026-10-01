#include "CQTest.h"
#include "Math/RandomStream.h"
#include "Weapon/ASHitboxMath.h"

namespace
{
	constexpr double DistanceTolerance = 1e-3;
	
	FASWorldCapsule MakeCapsule(const FVector& A, const FVector& B, float Radius)
	{
		FASWorldCapsule Capsule;
		Capsule.A = A;
		Capsule.B = B;
		Capsule.Radius = Radius;
		return Capsule;
	}
	
	FASWorldCapsule UprightCapsule()
	{
		return MakeCapsule(FVector(0.f,0.f,-50.f), FVector(0.f,0.f,50.f), 20.f);
	}
	
	FASWorldCapsule RandomCapsule(FRandomStream& Random, const FVector& Root)
	{
		const FVector A = Root + Random.VRand() * Random.FRandRange(0.f, 80.f);
		const FVector B = A + Random.VRand() * Random.FRandRange(0.f, 60.f);
		return MakeCapsule(A, B, Random.FRandRange(3.f, 20.f));
	}
}

TEST_CLASS(FASHitboxMathTests, "ArenaShooter.Weapon.HitboxMath")
{
	TEST_METHOD(RaySphere_FromOutside_HitsNearSurface)
	{
		const double Distance = ASHitboxMath::RaySphere(FVector(-100., 0., 0.), FVector::ForwardVector, FVector::ZeroVector, 10.);
		ASSERT_THAT(IsNear(90., Distance, DistanceTolerance));
	}
	
	TEST_METHOD(RaySphere_StartingInside_HitsAtZero)
	{
		ASSERT_THAT(IsTrue(ASHitboxMath::RaySphere(FVector(3., 0., 0.), FVector::ForwardVector, FVector::ZeroVector, 10.) == 0.));
	}

	TEST_METHOD(RaySphere_PointingAway_Misses)
	{
		ASSERT_THAT(IsTrue(ASHitboxMath::RaySphere(FVector(-100., 0., 0.), FVector::BackwardVector, FVector::ZeroVector, 10.) < 0.));
	}

	TEST_METHOD(RaySphere_PassingBeside_Misses)
	{
		ASSERT_THAT(IsTrue(ASHitboxMath::RaySphere(FVector(-100., 0., 11.), FVector::ForwardVector, FVector::ZeroVector, 10.) < 0.));
	}

	TEST_METHOD(RayCapsule_FromTheSide_HitsTheCylinder)
	{
		const double Distance = ASHitboxMath::RayCapsule(FVector(-100., 0., 0.), FVector::ForwardVector, UprightCapsule());
		ASSERT_THAT(IsNear(80., Distance, DistanceTolerance));
	}

	TEST_METHOD(RayCapsule_AlongTheAxis_HitsTheNearCap)
	{
		const double Distance = ASHitboxMath::RayCapsule(FVector(0., 0., 200.), FVector::DownVector, UprightCapsule());
		ASSERT_THAT(IsNear(130., Distance, DistanceTolerance));
	}

	TEST_METHOD(RayCapsule_PastTheSegmentEnd_HitsTheRoundedCapNotTheCylinder)
	{
		// The infinite cylinder is hit 80 cm out, but that point lies beyond B: only B's sphere counts.
		const double Distance = ASHitboxMath::RayCapsule(FVector(-100., 0., 60.), FVector::ForwardVector, UprightCapsule());
		ASSERT_THAT(IsNear(100. - FMath::Sqrt(300.), Distance, DistanceTolerance));
	}
	
	TEST_METHOD(RayCapsule_StartingInside_HitsAtZero)
	{
		ASSERT_THAT(IsTrue(ASHitboxMath::RayCapsule(FVector(5., 0., 10.), FVector::ForwardVector, UprightCapsule()) == 0.));
	}

	TEST_METHOD(RayCapsule_ParallelToTheAxisOutsideTheRadius_Misses)
	{
		ASSERT_THAT(IsTrue(ASHitboxMath::RayCapsule(FVector(30., 0., -200.), FVector::UpVector, UprightCapsule()) < 0.));
	}

	TEST_METHOD(RayCapsule_WithZeroLength_ActsAsASphere)
	{
		const FASWorldCapsule Point = MakeCapsule(FVector::ZeroVector, FVector::ZeroVector, 10.f);
		ASSERT_THAT(IsNear(90., ASHitboxMath::RayCapsule(FVector(-100., 0., 0.), FVector::ForwardVector, Point), DistanceTolerance));
	}

	TEST_METHOD(NearestCapsuleHit_PicksTheClosestNotTheFirst)
	{
		const TArray<FASWorldCapsule> Capsules =
		{
			MakeCapsule(FVector(200., 0., -50.), FVector(200., 0., 50.), 10.f),
			MakeCapsule(FVector(100., 0., -50.), FVector(100., 0., 50.), 10.f),
		};
		const FASCapsuleHit Hit = ASHitboxMath::NearestCapsuleHit(FVector::ZeroVector, FVector::ForwardVector, 1000., Capsules);
		ASSERT_THAT(AreEqual(1, Hit.Capsule));
		ASSERT_THAT(IsNear(90., Hit.Distance, DistanceTolerance));
	}

	TEST_METHOD(NearestCapsuleHit_BeyondMaxDistance_Misses)
	{
		const TArray<FASWorldCapsule> Capsules = { MakeCapsule(FVector(100., 0., -50.), FVector(100., 0., 50.), 10.f) };
		const FASCapsuleHit Hit = ASHitboxMath::NearestCapsuleHit(FVector::ZeroVector, FVector::ForwardVector, 50., Capsules);
		ASSERT_THAT(AreEqual(int32(INDEX_NONE), Hit.Capsule));
	}

	TEST_METHOD(MakeGroup_SphereContainsEveryCapsuleInItsRange)
	{
		FRandomStream Random(7);
		TArray<FASWorldCapsule> Capsules;
		for (int32 Index = 0; Index < 10; ++Index)
		{
			Capsules.Add(RandomCapsule(Random, FVector(500., -300., 100.)));
		}

		const FASHitboxGroup Group = ASHitboxMath::MakeGroup(Capsules, 3, 4);
		ASSERT_THAT(AreEqual(3, Group.FirstCapsule));
		ASSERT_THAT(AreEqual(4, Group.NumCapsules));

		for (int32 Index = 3; Index < 7; ++Index)
		{
			const FASWorldCapsule& Capsule = Capsules[Index];
			for (int32 Sample = 0; Sample < 200; ++Sample)
			{
				// Somewhere along the segment, pushed out by the full radius.
				const FVector Point = FMath::Lerp(Capsule.A, Capsule.B, Random.FRand()) + Random.VRand() * Capsule.Radius;
				ASSERT_THAT(IsTrue(FVector::Dist(Group.Center, Point) <= Group.Radius + DistanceTolerance));
			}
		}
	}
	
	TEST_METHOD(NearestGroupHit_MatchesBruteForceOverEveryCapsule)
	{
		// The group spheres are only a broad phase: skipping a group must never change which capsule a ray hits.
		FRandomStream Random(20260928);
		TArray<FASWorldCapsule> Capsules;
		TArray<FASHitboxGroup> Groups;
		for (int32 Character = 0; Character < 6; ++Character)
		{
			const FVector Root = Random.VRand() * Random.FRandRange(200.f, 2000.f);
			const int32 First = Capsules.Num();
			const int32 Num = Random.RandRange(1, 8);
			for (int32 Index = 0; Index < Num; ++Index)
			{
				Capsules.Add(RandomCapsule(Random, Root));
			}
			Groups.Add(ASHitboxMath::MakeGroup(Capsules, First, Num));
		}

		int32 NumHits = 0;
		for (int32 Ray = 0; Ray < 2000; ++Ray)
		{
			// Aim near a random capsule, so most rays hit something instead of flying off into space.
			const FASWorldCapsule& Target = Capsules[Random.RandHelper(Capsules.Num())];
			const FVector Aim = FMath::Lerp(Target.A, Target.B, Random.FRand()) + Random.VRand() * Random.FRandRange(0.f, 2.f * Target.Radius);
			const FVector Origin = Random.VRand() * Random.FRandRange(0.f, 3000.f);
			const FVector Dir = (Aim - Origin).GetSafeNormal();
			const double MaxDistance = Random.FRandRange(1000.f, 8000.f);

			const FASCapsuleHit Expected = ASHitboxMath::NearestCapsuleHit(Origin, Dir, MaxDistance, Capsules);
			const FASCapsuleHit Actual = ASHitboxMath::NearestGroupHit(Origin, Dir, MaxDistance, Groups, Capsules);
			ASSERT_THAT(AreEqual(Expected.Capsule, Actual.Capsule, FString::Printf(TEXT("Ray %d hit a different capsule"), Ray)));
			if (Expected.Capsule != INDEX_NONE)
			{
				ASSERT_THAT(IsNear(Expected.Distance, Actual.Distance, DistanceTolerance));
				++NumHits;
			}
		}
		
		// Guards the test itself: a generator that made every ray miss would pass without checking anything.
		ASSERT_THAT(IsTrue(NumHits > 400, TEXT("Too few rays hit anything to compare")));
	}
};