#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "AbilitySystemComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "AbilitySystem/Attributes/ASCombatAttributeSet.h"
#include "Helpers/ASNetworkTestSession.h"
#include "Helpers/ASTestMatch.h"
#include "Helpers/ASTestPlayer.h"
#include "Inventory/ASInventoryComponent.h"
#include "System/ASGameplayTags.h"
#include "Weapon/ASWeaponInstance.h"

namespace
{
	const TCHAR* TestMap = TEXT("/ArenaShooterTests/L_Test");

	// Each way, so a 100 ms round trip: enough for prediction and lag compensation to matter.
	constexpr int32 OneWayLagMs = 50;

	bool HasPistol(const APawn* Pawn)
	{
		const UASInventoryComponent* Inventory = ASTestPlayer::GetInventory(Pawn);
		return Inventory && Inventory->GetActiveInstance() && ASTestPlayer::GetWeapon(Pawn, FASGameplayTags::Weapon_Pistol);
	}

	/** Pistol out, and not mid-switch: the equip lock rejects shots until it ends. */
	bool IsHoldingPistolReady(const APawn* Pawn)
	{
		const UASInventoryComponent* Inventory = ASTestPlayer::GetInventory(Pawn);
		const UASWeaponInstance* Active = Inventory ? Inventory->GetActiveInstance() : nullptr;
		const UAbilitySystemComponent* AbilitySystem = ASTestPlayer::GetAbilitySystem(Pawn);
		return Active && Active->GetWeaponTag().MatchesTagExact(FASGameplayTags::Weapon_Pistol)
			&& AbilitySystem && !AbilitySystem->HasMatchingGameplayTag(FASGameplayTags::Ability_Weapon_IsChanging);
	}

	float GetHealthAndShield(const APawn* Pawn)
	{
		const UASCombatAttributeSet* Attributes = ASTestPlayer::GetAttributes(Pawn);
		return Attributes ? Attributes->GetHealth() + Attributes->GetShield() : 0.f;
	}
}

/**
 * A listen server and one client in PIE on L_Test, 100 ms apart. The client plays through its own predicted
 * abilities; each test checks that the server ends up agreeing with what the client did. Editor-only.
 */
TEST_CLASS_WITH_FLAGS(FASNetworkTests, "ArenaShooter.Network", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
{
	FASNetworkTestSession Session{ *TestRunner, TestCommandBuilder };
	FASWarmupSkip WarmupSkip;

	APawn* SettledClientPawn = nullptr;
	double SettledSince = 0.;

	int32 TargetSlot = INDEX_NONE;
	UASWeaponInstance* ClientPistol = nullptr;
	UASWeaponInstance* ServerPistol = nullptr;
	int32 AmmoBefore = 0;
	int32 FireCost = 0;
	int32 LowestClientAmmo = 0;
	double ServerConfirmedAt = 0.;
	float HostHealthBefore = 0.f;

	BEFORE_EACH()
	{
		ASTestMatch::ExpectEngineNoise(*TestRunner);
		ASSERT_THAT(IsTrue(WarmupSkip.Begin(), TEXT("The game has no AS.Match.WarmupOverride")));

		Session.Start(TestMap, OneWayLagMs);
		TestCommandBuilder.Until(TEXT("Both players are spawned for the match"), [this] { return ArePlayersSettled(); }, FTimespan::FromSeconds(30));
	}

	AFTER_EACH()
	{
		WarmupSkip.End();
	}

	/** Both machines are in the match with both players armed, and the client's pawn has stopped changing. */
	bool ArePlayersSettled()
	{
		const AGameStateBase* ServerState = Session.GetServerWorld() ? Session.GetServerWorld()->GetGameState() : nullptr;
		const AGameStateBase* ClientState = Session.GetClientWorld() ? Session.GetClientWorld()->GetGameState() : nullptr;
		if (!ServerState || !ServerState->HasMatchStarted() || !ClientState || !ClientState->HasMatchStarted())
		{
			return false;
		}

		APawn* ClientPawn = Session.GetClientOnClient();
		if (!HasPistol(Session.GetHostOnServer()) || !HasPistol(Session.GetClientOnServer()) || !HasPistol(ClientPawn) || !Session.GetHostOnClient())
		{
			return false;
		}

		// The match start respawns everyone, and the client may still be looking at its old pawn for a moment.
		const double Now = FPlatformTime::Seconds();
		if (ClientPawn != SettledClientPawn)
		{
			SettledClientPawn = ClientPawn;
			SettledSince = Now;
			return false;
		}
		return Now - SettledSince >= 0.5;
	}

	/** Queues: the client switches to its pistol, then both machines have it out and neither is mid-switch. */
	void EquipPistolOnClient()
	{
		TestCommandBuilder
			.Do(TEXT("The client switches to the pistol"), [this]
			{
				UASInventoryComponent* Inventory = ASTestPlayer::GetInventory(Session.GetClientOnClient());
				ASSERT_THAT(IsNotNull(Inventory));
				Inventory->RequestSwitch(Inventory->FindSlotByTag(FASGameplayTags::Weapon_Pistol));
			})
			.Until(TEXT("Both machines have the pistol out"), [this]
			{
				return IsHoldingPistolReady(Session.GetClientOnClient()) && IsHoldingPistolReady(Session.GetClientOnServer());
			}, FTimespan::FromSeconds(10));
	}

	/** Queues: the client pulls the trigger every frame until its pistol's ammo drops. */
	void FirePistolOnClient()
	{
		TestCommandBuilder
			.Then(TEXT("Note the client's ammo"), [this]
			{
				ClientPistol = ASTestPlayer::GetWeapon(Session.GetClientOnClient(), FASGameplayTags::Weapon_Pistol);
				ASSERT_THAT(IsNotNull(ClientPistol));
				AmmoBefore = ClientPistol->GetAmmo();
				ASSERT_THAT(IsTrue(AmmoBefore >= ClientPistol->GetFireCost(), TEXT("The pistol starts without enough ammo to fire")));
			})
			.Until(TEXT("The client fires"), [this]
			{
				if (!ClientPistol)
				{
					return true;
				}
				ASTestPlayer::PullTrigger(Session.GetClientOnClient());
				return ClientPistol->GetAmmo() < AmmoBefore;
			}, FTimespan::FromSeconds(5));
	}

	TEST_METHOD(ClientWeaponSwitch_IsPredictedThenAppliedOnTheServer)
	{
		TestCommandBuilder
			.Do(TEXT("The client asks for another weapon"), [this]
			{
				UASInventoryComponent* Inventory = ASTestPlayer::GetInventory(Session.GetClientOnClient());
				ASSERT_THAT(IsNotNull(Inventory));
				for (int32 Slot = 0; Slot < Inventory->GetCapacity() && TargetSlot == INDEX_NONE; ++Slot)
				{
					if (Slot != Inventory->GetActiveSlot() && Inventory->IsSlotFilled(Slot))
					{
						TargetSlot = Slot;
					}
				}
				ASSERT_THAT(IsTrue(TargetSlot != INDEX_NONE, TEXT("The loadout has only one weapon")));

				Inventory->RequestSwitch(TargetSlot);
				// Predicted: the client holds the new weapon before the server has even heard about it.
				ASSERT_THAT(IsTrue(Inventory->GetActiveInstance() == Inventory->GetInstanceAtSlot(TargetSlot)));
			})
			.Until(TEXT("The server switches the client's weapon too"), [this]
			{
				const UASInventoryComponent* Inventory = ASTestPlayer::GetInventory(Session.GetClientOnServer());
				return Inventory && Inventory->GetActiveSlot() == TargetSlot;
			}, FTimespan::FromSeconds(5))
			// Long enough for the server's confirmation to reach the client and be reconciled.
			.WaitDelay(FTimespan::FromMilliseconds(500))
			.Then(TEXT("The client never snapped back"), [this]
			{
				const UASInventoryComponent* Inventory = ASTestPlayer::GetInventory(Session.GetClientOnClient());
				ASSERT_THAT(IsNotNull(Inventory));
				ASSERT_THAT(IsTrue(Inventory->GetActiveInstance() == Inventory->GetInstanceAtSlot(TargetSlot)));
			});
	}

	TEST_METHOD(ClientShot_SpendsAmmoOnceOnBothMachines)
	{
		EquipPistolOnClient();
		TestCommandBuilder.Then(TEXT("Both machines agree on the ammo"), [this]
		{
			ServerPistol = ASTestPlayer::GetWeapon(Session.GetClientOnServer(), FASGameplayTags::Weapon_Pistol);
			const UASWeaponInstance* Pistol = ASTestPlayer::GetWeapon(Session.GetClientOnClient(), FASGameplayTags::Weapon_Pistol);
			ASSERT_THAT(IsNotNull(ServerPistol));
			ASSERT_THAT(IsNotNull(Pistol));
			ASSERT_THAT(AreEqual(ServerPistol->GetAmmo(), Pistol->GetAmmo(), TEXT("The machines disagree about ammo before the shot")));
			FireCost = ServerPistol->GetFireCost();
		});
		FirePistolOnClient();
		TestCommandBuilder
			.Then(TEXT("The client's count drops at once, predicted"), [this]
			{
				ASSERT_THAT(IsNotNull(ClientPistol));
				ASSERT_THAT(AreEqual(AmmoBefore - FireCost, ClientPistol->GetAmmo()));
				LowestClientAmmo = ClientPistol->GetAmmo();
				ServerConfirmedAt = 0.;
			})
			.Until(TEXT("The server confirms, and its count reaches the client"), [this]
			{
				if (!ClientPistol || !ServerPistol)
				{
					return true;
				}
				LowestClientAmmo = FMath::Min(LowestClientAmmo, ClientPistol->GetAmmo());

				const double Now = FPlatformTime::Seconds();
				if (ServerConfirmedAt == 0. && ServerPistol->GetAmmo() < AmmoBefore)
				{
					ServerConfirmedAt = Now;
				}
				// Several round trips after the server's count changes, so its replicated ammo has arrived.
				return ServerConfirmedAt > 0. && Now - ServerConfirmedAt >= 0.5;
			}, FTimespan::FromSeconds(5))
			.Then(TEXT("One shot, counted once, everywhere"), [this]
			{
				const int32 Expected = AmmoBefore - FireCost;
				ASSERT_THAT(AreEqual(Expected, ServerPistol->GetAmmo(), TEXT("The server")));
				ASSERT_THAT(AreEqual(Expected, ClientPistol->GetAmmo(), TEXT("The client, once the server's ammo arrived")));
				ASSERT_THAT(AreEqual(Expected, LowestClientAmmo, TEXT("The client counted the shot twice at some point")));
			});
	}

	TEST_METHOD(ClientHitOnTheHost_DamagesThemOnTheServer)
	{
		EquipPistolOnClient();
		TestCommandBuilder.Then(TEXT("The client aims at the host"), [this]
		{
			HostHealthBefore = GetHealthAndShield(Session.GetHostOnServer());
			ASSERT_THAT(IsTrue(HostHealthBefore > 0.f));

			APawn* Shooter = Session.GetClientOnClient();
			const APawn* Target = Session.GetHostOnClient();
			ASSERT_THAT(IsNotNull(Shooter));
			ASSERT_THAT(IsNotNull(Target));
			ASSERT_THAT(IsNotNull(Shooter->GetController()));

			// Hitscan aims along the control rotation from the eyes.
			FVector Eyes;
			FRotator Unused;
			Shooter->GetActorEyesViewPoint(Eyes, Unused);
			Shooter->GetController()->SetControlRotation((Target->GetActorLocation() - Eyes).Rotation());
		});
		FirePistolOnClient();
		// The server retraces the client's ray against where the host was 100 ms earlier, and applies the damage.
		TestCommandBuilder.Until(TEXT("The server damages the host"), [this]
		{
			return GetHealthAndShield(Session.GetHostOnServer()) < HostHealthBefore;
		}, FTimespan::FromSeconds(5));
	}
};

#endif // WITH_AUTOMATION_TESTS