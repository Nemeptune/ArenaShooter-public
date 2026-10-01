#include "CQTest.h"
#include "Components/ActorTestSpawner.h"
#include "GameFramework/Pawn.h"
#include "Helpers/ASTestActors.h"
#include "Weapon/ASWeaponDefinition.h"
#include "Weapon/ASWeaponInstance.h"

TEST_CLASS(FASWeaponRefireTests, "ArenaShooter.Weapon.Refire")
{
	static constexpr float FireInterval = 0.5f;

	FActorTestSpawner Spawner;

	UASWeaponInstance* MakeWeapon(UObject& Owner, float Interval = FireInterval)
	{
		UASWeaponDefinition* Definition = NewObject<UASWeaponDefinition>(GetTransientPackage());
		Definition->FireInterval = Interval;

		UASWeaponInstance* Weapon = NewObject<UASWeaponInstance>(&Owner);
		Weapon->Initialize(Definition);
		return Weapon;
	}

	/** The server's copy of a remote client's weapon: the owner has authority but no local controller. */
	UASWeaponInstance* MakeServerSideWeapon(float Interval = FireInterval)
	{
		return MakeWeapon(Spawner.SpawnActor<AActor>(), Interval);
	}

	/** The local player's weapon: its pawn is possessed by a controller on this machine. */
	UASWeaponInstance* MakeLocalWeapon()
	{
		APawn& Pawn = Spawner.SpawnActor<APawn>();
		Spawner.SpawnActor<AASTestController>().Possess(&Pawn);
		return MakeWeapon(Pawn);
	}

	void AdvanceTime(float Seconds)
	{
		constexpr float Step = 0.1f;
		UWorld& World = Spawner.GetWorld();
		for (; Seconds > Step; Seconds -= Step)
		{
			World.Tick(LEVELTICK_TimeOnly, Step);
		}
		World.Tick(LEVELTICK_TimeOnly, Seconds);
	}

	TEST_METHOD(BeforeTheFirstShot_CanFire)
	{
		ASSERT_THAT(IsTrue(MakeServerSideWeapon()->CanFire()));
	}

	TEST_METHOD(RightAfterAShot_CannotFire)
	{
		UASWeaponInstance* Weapon = MakeServerSideWeapon();
		Weapon->MarkFired();
		ASSERT_THAT(IsFalse(Weapon->CanFire()));
	}

	TEST_METHOD(LocalPlayer_WaitsTheFullInterval)
	{
		UASWeaponInstance* Weapon = MakeLocalWeapon();
		Weapon->MarkFired();
		AdvanceTime(0.48f);
		ASSERT_THAT(IsFalse(Weapon->CanFire(), TEXT("20 ms early")));
		AdvanceTime(0.03f);
		ASSERT_THAT(IsTrue(Weapon->CanFire(), TEXT("10 ms late")));
	}

	TEST_METHOD(ServerForRemoteClient_AllowsThirtyMillisecondsOfJitter)
	{
		UASWeaponInstance* Weapon = MakeServerSideWeapon();
		Weapon->MarkFired();
		AdvanceTime(0.46f);
		ASSERT_THAT(IsFalse(Weapon->CanFire(), TEXT("40 ms early is beyond the tolerance")));
		AdvanceTime(0.02f);
		ASSERT_THAT(IsTrue(Weapon->CanFire(), TEXT("20 ms early is inside the tolerance")));
	}

	TEST_METHOD(WeaponWithoutInterval_AlwaysFires)
	{
		UASWeaponInstance* Weapon = MakeServerSideWeapon(0.f);
		Weapon->MarkFired();
		ASSERT_THAT(IsTrue(Weapon->CanFire()));
	}
};