#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogAS, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAS_Ability, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAS_Input, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAS_Weapon, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAS_Movement, Log, All);

ARENASHOOTER_API DECLARE_LOG_CATEGORY_EXTERN(LogProjectiles, Log, All);

#define PROJECTILE_LOG(Verbosity, Format, ...) \
{ \
	UE_LOG(LogProjectiles, Verbosity, Format, ##__VA_ARGS__); \
}
