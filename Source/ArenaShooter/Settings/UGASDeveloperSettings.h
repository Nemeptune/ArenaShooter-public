#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UGASDeveloperSettings.generated.h"

UENUM(BlueprintType)
enum class EProjectileDebugMode : uint8
{
	None,
	PredictedVersusClient,
	ClientVersusServer,
	All
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GAS Developer Settings"))
class UGASDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UGASDeveloperSettings();

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging|Projectile")
	EProjectileDebugMode ProjectileDebugMode;
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging|Projectile")
	bool bWaitForLinkage;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging|Projectile")
	bool bDrawSpawnPosition;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging|Projectile")
	bool bDrawFinalPosition;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging|Projectile")
	bool bLogCorrection;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging|Projectile", Meta = (UIMin = "0.1", UIMax = "5.0", ClampMin = "0.1", ClampMax = "5.0", Multiple = "0.1", Delta = "0.1"))
	float DrawFrequency;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging|Projectile", Meta = (UIMin = "0.1", ClampMin = "0.1", Units = "seconds"))
	float DrawTime;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging|Projectile")
	FColor ServerProjectileColor;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging|Projectile")
	FColor ClientAuthoritativeProjectileColor;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging|Projectile")
	FColor ClientFakeProjectileColor;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging|Projectile")
	FColor SyncedColor;
};
