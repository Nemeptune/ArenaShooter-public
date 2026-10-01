#include "CQTest.h"
#include "Math/RandomStream.h"
#include "Weapon/ASShotValidation.h"

namespace
{
	/** Unit direction YawDeg degrees to the side of straight ahead (+X). */
	FVector DirectionAtYaw(float YawDeg)
	{
		return FRotator(0.f, YawDeg, 0.f).Vector();
	}
}

TEST_CLASS(FASShotValidationTests, "ArenaShooter.Weapon.ShotValidation")
{
	TEST_METHOD(SingleRay_IsValid)
	{
		const TArray<FVector> Directions = { FVector::ForwardVector };
		ASSERT_THAT(AreEqual(EASShotVerdict::Valid, ASShotValidation::CheckBullets(Directions, 1, 0.f)));
	}

	TEST_METHOD(NoBullets_IsValid)
	{
		const TArray<FVector> Directions;
		ASSERT_THAT(AreEqual(EASShotVerdict::Valid, ASShotValidation::CheckBullets(Directions, 1, 0.f)));
	}

	TEST_METHOD(ExactlyTheWeaponsBulletCount_IsValid)
	{
		const TArray<FVector> Directions = { FVector::ForwardVector, FVector::ForwardVector, FVector::ForwardVector };
		ASSERT_THAT(AreEqual(EASShotVerdict::Valid, ASShotValidation::CheckBullets(Directions, 3, 0.f)));
	}

	TEST_METHOD(MoreBulletsThanTheWeaponFires_IsTooManyBullets)
	{
		const TArray<FVector> Directions = { FVector::ForwardVector, FVector::ForwardVector, FVector::ForwardVector, FVector::ForwardVector };
		ASSERT_THAT(AreEqual(EASShotVerdict::TooManyBullets, ASShotValidation::CheckBullets(Directions, 3, 0.f)));
	}

	TEST_METHOD(ZeroFirstDirection_IsNotARay)
	{
		const TArray<FVector> Directions = { FVector::ZeroVector };
		ASSERT_THAT(AreEqual(EASShotVerdict::NotARay, ASShotValidation::CheckBullets(Directions, 1, 0.f)));
	}

	TEST_METHOD(ZeroLaterDirection_IsNotARay)
	{
		const TArray<FVector> Directions = { FVector::ForwardVector, FVector::ZeroVector };
		ASSERT_THAT(AreEqual(EASShotVerdict::NotARay, ASShotValidation::CheckBullets(Directions, 2, 10.f)));
	}

	TEST_METHOD(PelletsAtOppositeEdgesOfTheCone_AreValid)
	{
		// Half-angle 5: pellets on opposite edges are 10 degrees apart, the widest an honest shot gets.
		const TArray<FVector> Directions = { DirectionAtYaw(5.f), DirectionAtYaw(-5.f) };
		ASSERT_THAT(AreEqual(EASShotVerdict::Valid, ASShotValidation::CheckBullets(Directions, 2, 5.f)));
	}

	TEST_METHOD(PelletsWiderThanTheCone_AreOutsideSpread)
	{
		// 10 degrees of cone plus 1 of tolerance: 12 apart is out.
		const TArray<FVector> Directions = { DirectionAtYaw(6.f), DirectionAtYaw(-6.f) };
		ASSERT_THAT(AreEqual(EASShotVerdict::OutsideSpread, ASShotValidation::CheckBullets(Directions, 2, 5.f)));
	}

	TEST_METHOD(NoSpreadWeapon_ToleratesWireRounding)
	{
		const TArray<FVector> Directions = { FVector::ForwardVector, DirectionAtYaw(0.5f) };
		ASSERT_THAT(AreEqual(EASShotVerdict::Valid, ASShotValidation::CheckBullets(Directions, 2, 0.f)));
	}

	TEST_METHOD(NoSpreadWeapon_RejectsASecondAim)
	{
		const TArray<FVector> Directions = { FVector::ForwardVector, DirectionAtYaw(2.f) };
		ASSERT_THAT(AreEqual(EASShotVerdict::OutsideSpread, ASShotValidation::CheckBullets(Directions, 2, 0.f)));
	}

	TEST_METHOD(HonestShotgunShots_AreNeverRejected)
	{
		// Pellets made the way PerformLocalTargeting makes them: random directions in a cone around the aim.
		FRandomStream Random(29092026);
		constexpr int32 BulletsPerShot = 12;
		constexpr float SpreadHalfAngleDeg = 8.f;
		for (int32 Shot = 0; Shot < 1000; ++Shot)
		{
			const FVector Aim = Random.VRand();
			TArray<FVector> Directions;
			for (int32 Pellet = 0; Pellet < BulletsPerShot; ++Pellet)
			{
				Directions.Add(Random.VRandCone(Aim, FMath::DegreesToRadians(SpreadHalfAngleDeg)));
			}

			const EASShotVerdict Verdict = ASShotValidation::CheckBullets(Directions, BulletsPerShot, SpreadHalfAngleDeg);
			ASSERT_THAT(AreEqual(EASShotVerdict::Valid, Verdict, FString::Printf(TEXT("Shot %d was rejected: %s"), Shot, LexToString(Verdict))));
		}
	}
};