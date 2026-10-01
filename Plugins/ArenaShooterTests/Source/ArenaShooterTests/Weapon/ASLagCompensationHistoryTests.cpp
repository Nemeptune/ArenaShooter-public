#include "CQTest.h"
#include "Weapon/ASLagCompensationSubsystem.h"

namespace
{
	constexpr double DistanceTolerance = 1e-3;

	/** One upright capsule standing at X, so a sample's X shows which frames were blended and by how much. */
	FASHitboxFrame MakeFrame(double Time, double X)
	{
		FASHitboxFrame Frame;
		Frame.Time = Time;
		FASWorldCapsule& Capsule = Frame.Capsules.AddDefaulted_GetRef();
		Capsule.A = FVector(X, 0., 0.);
		Capsule.B = FVector(X, 0., 100.);
		Capsule.Radius = 10.f;
		return Frame;
	}
}

TEST_CLASS(ASLagCompensationHistoryTests, "ArenaShooter.Weapon.LagCompensation.History")
{
	FASTrackedCharacter Tracked;

	BEFORE_EACH()
	{
		// A ring that has wrapped: slot 0 was just overwritten with the newest frame, so the oldest sits in slot 1.
		Tracked.Frames = { MakeFrame(0.3, 30.), MakeFrame(0.1, 10.), MakeFrame(0.2, 20.) };
		Tracked.NewestFrame = 0;
	}

	TArray<FASWorldCapsule> SampleAt(double Time) const
	{
		TArray<FASWorldCapsule> Capsules;
		Tracked.AppendCapsulesAt(Time, Capsules);
		return Capsules;
	}

	TEST_METHOD(BetweenTwoFrames_BlendsThem)
	{
		const TArray<FASWorldCapsule> Capsules = SampleAt(0.15);
		ASSERT_THAT(AreEqual(1, Capsules.Num()));
		ASSERT_THAT(IsNear(15., Capsules[0].A.X, DistanceTolerance));
	}

	TEST_METHOD(AcrossTheRingWrap_BlendsTheNewestWithTheOneBefore)
	{
		const TArray<FASWorldCapsule> Capsules = SampleAt(0.25);
		ASSERT_THAT(AreEqual(1, Capsules.Num()));
		ASSERT_THAT(IsNear(25., Capsules[0].A.X, DistanceTolerance));
	}

	TEST_METHOD(ExactlyOnAFrame_ReturnsThatFrame)
	{
		const TArray<FASWorldCapsule> Capsules = SampleAt(0.2);
		ASSERT_THAT(AreEqual(1, Capsules.Num()));
		ASSERT_THAT(IsNear(20., Capsules[0].A.X, DistanceTolerance));
	}

	TEST_METHOD(AfterTheNewestFrame_ReturnsTheNewest)
	{
		const TArray<FASWorldCapsule> Capsules = SampleAt(1.0);
		ASSERT_THAT(AreEqual(1, Capsules.Num()));
		ASSERT_THAT(IsNear(30., Capsules[0].A.X, DistanceTolerance));
	}

	TEST_METHOD(BeforeTheOldestFrame_ReturnsTheOldest)
	{
		const TArray<FASWorldCapsule> Capsules = SampleAt(0.0);
		ASSERT_THAT(AreEqual(1, Capsules.Num()));
		ASSERT_THAT(IsNear(10., Capsules[0].A.X, DistanceTolerance));
	}

	TEST_METHOD(SingleFrame_IsReturnedForAnyTime)
	{
		Tracked.Frames = { MakeFrame(0.2, 20.) };
		Tracked.NewestFrame = 0;
		const TArray<FASWorldCapsule> Capsules = SampleAt(0.1);
		ASSERT_THAT(AreEqual(1, Capsules.Num()));
		ASSERT_THAT(IsNear(20., Capsules[0].A.X, DistanceTolerance));
	}

	TEST_METHOD(NoFrames_AppendsNothing)
	{
		Tracked.Frames.Reset();
		Tracked.NewestFrame = INDEX_NONE;
		ASSERT_THAT(IsTrue(SampleAt(0.2).IsEmpty()));
	}

	TEST_METHOD(Sample_AppendsAfterWhatIsAlreadyThere)
	{
		// LineTraceRewound samples every character into one array and slices it by offset.
		TArray<FASWorldCapsule> Capsules;
		Capsules.AddDefaulted();
		Tracked.AppendCapsulesAt(0.15, Capsules);
		ASSERT_THAT(AreEqual(2, Capsules.Num()));
		ASSERT_THAT(IsNear(15., Capsules[1].A.X, DistanceTolerance));
	}
};