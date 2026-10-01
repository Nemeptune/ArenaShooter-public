#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "AbilitySystemComponent.h"
#include "Components/MapTestSpawner.h"
#include "Editor.h"
#include "GameplayEffect.h"
#include "AbilitySystem/Attributes/ASCombatAttributeSet.h"
#include "Helpers/ASTestEffects.h"
#include "Helpers/ASTestMatch.h"
#include "Helpers/ASTestPlayer.h"
#include "Inventory/ASInventoryComponent.h"
#include "System/ASGameplayTags.h"
#include "Weapon/ASWeaponInstance.h"

namespace
{
	const TCHAR* TestMapDirectory = TEXT("/ArenaShooterTests");
	const TCHAR* TestMapName = TEXT("L_Test");
	constexpr float AttributeTolerance = 1e-3f;
}

/**
 * Loads L_Test in PIE with the real game mode, character and loadout, and plays it: the tests drive the
 * player the way input and damage do in a match. Editor-only, because they start PIE.
 */
TEST_CLASS_WITH_FLAGS(FASMapTests, "ArenaShooter.Map", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
{
	TUniquePtr<FMapTestSpawner> Spawner;
	FASWarmupSkip WarmupSkip;
	APawn* Player = nullptr;

	UASWeaponInstance* Weapon = nullptr;
	int32 AmmoBefore = 0;
	APawn* DeadPawn = nullptr;

	BEFORE_EACH()
	{
		ASTestMatch::ExpectEngineNoise(*TestRunner);
		ASSERT_THAT(IsTrue(WarmupSkip.Begin(), TEXT("The game has no AS.Match.WarmupOverride")));

		Spawner = MakeUnique<FMapTestSpawner>(TestMapDirectory, TestMapName);
		Spawner->AddWaitUntilLoadedCommand(TestRunner);

		TestCommandBuilder
			.StartWhen([this] { return Spawner->FindFirstPlayerPawn() != nullptr; })
			.Then([this] { Player = Spawner->FindFirstPlayerPawn(); });
	}

	AFTER_EACH()
	{
		WarmupSkip.End();
		if (GEditor)
		{
			GEditor->RequestEndPlayMap();
		}
	}

	TEST_METHOD(Player_SpawnsWithFullHealthAndAnEquippedWeapon)
	{
		TestCommandBuilder.Then([this]
		{
			const UASCombatAttributeSet* Attributes = ASTestPlayer::GetAttributes(Player);
			ASSERT_THAT(IsNotNull(Attributes, TEXT("The player has no combat attributes")));
			ASSERT_THAT(IsTrue(Attributes->GetMaxHealth() > 0.f));
			ASSERT_THAT(IsNear(Attributes->GetMaxHealth(), Attributes->GetHealth(), AttributeTolerance));

			const UASInventoryComponent* Inventory = ASTestPlayer::GetInventory(Player);
			ASSERT_THAT(IsNotNull(Inventory, TEXT("The player has no inventory")));
			ASSERT_THAT(IsNotNull(Inventory->GetActiveInstance(), TEXT("The loadout didn't equip a weapon")));
			ASSERT_THAT(IsTrue(Inventory->FindSlotByTag(FASGameplayTags::Weapon_Pistol) != INDEX_NONE, TEXT("The loadout has no pistol")));
		});
	}

	TEST_METHOD(FiringThePistol_SpendsItsFireCost)
	{
		TestCommandBuilder
			.Then([this]
			{
				UASInventoryComponent* Inventory = ASTestPlayer::GetInventory(Player);
				ASSERT_THAT(IsNotNull(Inventory, TEXT("The player has no inventory")));
				const int32 PistolSlot = Inventory->FindSlotByTag(FASGameplayTags::Weapon_Pistol);
				ASSERT_THAT(IsTrue(PistolSlot != INDEX_NONE, TEXT("The loadout has no pistol")));

				Inventory->SetActiveSlotAuth(PistolSlot);
				Weapon = Inventory->GetActiveInstance();
				ASSERT_THAT(IsNotNull(Weapon));
				AmmoBefore = Weapon->GetAmmo();
				ASSERT_THAT(IsTrue(AmmoBefore >= Weapon->GetFireCost(), TEXT("The pistol starts without enough ammo to fire")));
			})
			// Pull the trigger every frame until a shot goes off: equipping can block firing for a moment.
			.Until([this]
			{
				if (!Weapon)
				{
					return true;
				}
				ASTestPlayer::PullTrigger(Player);
				return Weapon->GetAmmo() < AmmoBefore;
			}, FTimespan::FromSeconds(10))
			.Then([this]
			{
				ASSERT_THAT(IsNotNull(Weapon));
				ASSERT_THAT(AreEqual(AmmoBefore - Weapon->GetFireCost(), Weapon->GetAmmo()));
			});
	}

	TEST_METHOD(LethalDamage_RespawnsThePlayerWithFullHealthAndALoadout)
	{
		TestCommandBuilder
			.Then([this]
			{
				UAbilitySystemComponent* AbilitySystem = ASTestPlayer::GetAbilitySystem(Player);
				const UASCombatAttributeSet* Attributes = ASTestPlayer::GetAttributes(Player);
				ASSERT_THAT(IsNotNull(Attributes));

				// More than shield and health together, so the hit is lethal wherever the shield starts.
				const float Lethal = Attributes->GetMaxHealth() + Attributes->GetMaxShield() + 1.f;
				FGameplayEffectSpec Spec(ASTestEffects::MakeRawDamage(Lethal), AbilitySystem->MakeEffectContext(), 1.f);
				AbilitySystem->ApplyGameplayEffectSpecToSelf(Spec);
				DeadPawn = Player;
			})
			.Until([this]
			{
				const UAbilitySystemComponent* AbilitySystem = ASTestPlayer::GetAbilitySystem(Player);
				return AbilitySystem && AbilitySystem->HasMatchingGameplayTag(FASGameplayTags::Status_Death_Dead);
			})
			// The game mode respawns the player after its RespawnDelay, onto a fresh pawn.
			.Until([this]
			{
				APawn* Current = Spawner->FindFirstPlayerPawn();
				if (!Current || Current == DeadPawn)
				{
					return false;
				}
				Player = Current;
				const UAbilitySystemComponent* AbilitySystem = ASTestPlayer::GetAbilitySystem(Player);
				return AbilitySystem && !AbilitySystem->HasMatchingGameplayTag(FASGameplayTags::Status_Death_Dead);
			}, FTimespan::FromSeconds(15))
			.Then([this]
			{
				const UASCombatAttributeSet* Attributes = ASTestPlayer::GetAttributes(Player);
				ASSERT_THAT(IsNotNull(Attributes));
				ASSERT_THAT(IsNear(Attributes->GetMaxHealth(), Attributes->GetHealth(), AttributeTolerance));

				const UASInventoryComponent* Inventory = ASTestPlayer::GetInventory(Player);
				ASSERT_THAT(IsNotNull(Inventory));
				ASSERT_THAT(IsNotNull(Inventory->GetActiveInstance(), TEXT("The respawned player has no weapon")));
			});
	}
};

#endif 