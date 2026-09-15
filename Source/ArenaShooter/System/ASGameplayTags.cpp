#include "ASGameplayTags.h"

// Abilities
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Ability_Weapon,				"Ability.Weapon");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Ability_Melee,				"Ability.Melee");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Ability_Weapon_IsChanging,	"Ability.Weapon.IsChanging");

// Weapon state
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Weapon_IsFiring, "Weapon.IsFiring");

// Weapon type
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Weapon_None,			"Weapon.None");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Weapon_Pistol,			"Weapon.Pistol");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Weapon_Rifle,			"Weapon.Rifle");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Weapon_RocketLauncher,	"Weapon.RocketLauncher");

// Weapon GameplayCues
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::GameplayCue_Weapon_Fire, "GameplayCue.Weapon.Fire");

// Character death
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Status_Death,		"Status.Death");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Status_Death_Dying,	"Status.Death.Dying");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Status_Death_Dead,	"Status.Death.Dead");

// Gameplay events that drive the death/respawn ability
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Event_Death,		"Event.Death");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Event_Respawn,		"Event.Character.Respawned");

UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Event_Damage_Dealt,	"Event.Damage.Dealt");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Event_Damage_Taken,	"Event.Damage.Taken");

UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Data_Momentum,	"Data.Momentum");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Data_Damage,	"Data.Damage");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Data_Healing,	"Data.Healing");

UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Damage_Lethal,		"Damage.Lethal");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Damage_Crit,		"Damage.Crit");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Damage_Reflected,	"Damage.Reflected");

UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Gameplay_Zone,		"Gameplay.Zone");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Gameplay_Zone_Head, "Gameplay.Zone.Head");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Gameplay_Zone_Body, "Gameplay.Zone.Body");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Gameplay_Zone_Limb, "Gameplay.Zone.Limb");

UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Buff,            "Buff");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Buff_Strength,   "Buff.Strength");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Buff_Resistance, "Buff.Resistance");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Buff_Vampire,    "Buff.Vampire");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Buff_Reflect,    "Buff.Reflect");

// Ammo types
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Ammo_Bullet, "Ammo.Bullet");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Ammo_Energy, "Ammo.Energy");
UE_DEFINE_GAMEPLAY_TAG(FASGameplayTags::Ammo_Rocket, "Ammo.Rocket");