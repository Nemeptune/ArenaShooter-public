#include "CQTest.h"
#include "Components/ActorTestSpawner.h"
#include "Helpers/ASTestActors.h"

namespace
{
	constexpr double SpeedTolerance = 0.01;
}

TEST_CLASS(FASKnockbackTests, "ArenaShooter.Character.Knockback")
{
	FActorTestSpawner Spawner;
	UASTestMovementComponent* Movement = nullptr;

	BEFORE_EACH()
	{
		Movement = Spawner.SpawnActor<AASTestCharacter>().GetTestMovementComponent();

		// Pinned so the numbers below stay right if the defaults are retuned.
		Movement->Mass = 100.f;
		Movement->MaxWalkSpeed = 600.f;
		Movement->KnockbackDampingSpeed = 1200.f;
		Movement->MaxKnockbackHorizontalVelocity = 2000.f;
		Movement->MaxUndampedImpulseZ = 1500.f;
		Movement->MaxAdditiveKnockbackZ = 750.f;
	}

	/** Knocks the character by an impulse that would change its velocity by VelocityChange before any damping. */
	void Knock(const FVector& VelocityChange, bool bSelfInflicted = false)
	{
		Movement->AddDampedImpulse(VelocityChange * Movement->Mass, bSelfInflicted);
	}

	/** What the next move starts from: the current velocity plus what knockback queued. */
	FVector VelocityAfterKnockback() const
	{
		return Movement->Velocity + Movement->GetPendingImpulse();
	}

	TEST_METHOD(FromStandstill_ModestHit_LandsUndamped)
	{
		Knock(FVector(500., 0., 0.));
		ASSERT_THAT(IsNear(500., VelocityAfterKnockback().X, SpeedTolerance));
	}

	TEST_METHOD(FromStandstill_HugeHit_IsCapped)
	{
		Knock(FVector(5000., 0., 0.));
		ASSERT_THAT(IsNear(2000., VelocityAfterKnockback().Size2D(), SpeedTolerance));
	}

	TEST_METHOD(SideHitWhileMoving_IsCapped)
	{
		Movement->Velocity = FVector(1000., 0., 0.);
		Knock(FVector(0., 3000., 0.));
		ASSERT_THAT(IsNear(2000., VelocityAfterKnockback().Size2D(), SpeedTolerance));
	}

	TEST_METHOD(PushAlongMotionAboveDampingSpeed_IsDamped)
	{
		// 1500 is 1.25x the damping speed, so the 500 push shrinks to 400, then halves: +200.
		Movement->Velocity = FVector(1500., 0., 0.);
		Knock(FVector(500., 0., 0.));
		ASSERT_THAT(IsNear(1700., VelocityAfterKnockback().X, SpeedTolerance));
	}

	TEST_METHOD(PlayerFasterThanTheCap_KeepsTheirSpeed)
	{
		Movement->Velocity = FVector(2500., 0., 0.);
		Knock(FVector(0., 100., 0.));
		ASSERT_THAT(IsNear(2500., VelocityAfterKnockback().Size2D(), SpeedTolerance));
	}

	TEST_METHOD(OwnRocket_LiftsUndampedBelowItsCeiling)
	{
		Knock(FVector(0., 0., 1200.), true);
		ASSERT_THAT(IsNear(1200., VelocityAfterKnockback().Z, SpeedTolerance));
	}

	TEST_METHOD(EnemyRocket_LiftIsDampedAboveItsCeiling)
	{
		// 750 of the 1200 lands in full; the 450 above the ceiling lands at 62.5%: 750 / 1200 = 0.625.
		Knock(FVector(0., 0., 1200.));
		ASSERT_THAT(IsNear(1031.25, VelocityAfterKnockback().Z, SpeedTolerance));
	}

	TEST_METHOD(ZeroImpulse_QueuesNothing)
	{
		Knock(FVector::ZeroVector);
		ASSERT_THAT(IsTrue(Movement->GetPendingImpulse().IsZero()));
	}
};