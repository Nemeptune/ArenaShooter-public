#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "ASLogChannels.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/ASInventoryComponent.h"
#include "Weapon/ASWeaponInstance.h"
#include "ArenaShooter.h"
#include "Async/ParallelFor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Weapon/ASLagCompensationSubsystem.h"

#if !UE_BUILD_SHIPPING

/*
 * Solo network tests for PIE. Each window has its own console, and a command acts on the world of the
 * window it was typed in: AS.Test.Strafe in the target's window, AS.Test.AutoAim in the shooter's.
 */
namespace
{
	FTSTicker::FDelegateHandle StrafeHandle;
	FTSTicker::FDelegateHandle AutoAimHandle;

	/** Runs Tick every frame on World's first player controller, or stops it if it's already running. */
	void ToggleTicker(FTSTicker::FDelegateHandle& Handle, UWorld* World, TFunction<void(APlayerController&)> Tick)
	{
		if (Handle.IsValid())
		{
			FTSTicker::RemoveTicker(Handle);
			Handle.Reset();
			return;
		}

		TWeakObjectPtr<UWorld> WeakWorld = World;
		Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakWorld, Tick, &Handle](float)
		{
			const UWorld* LiveWorld = WeakWorld.Get();
			if (!LiveWorld)
			{
				Handle.Reset(); // PIE ended
				return false;
			}
			if (APlayerController* PC = LiveWorld->GetFirstPlayerController())
			{
				Tick(*PC);
			}
			return true;
		}));
	}
}

static FAutoConsoleCommandWithWorldAndArgs GStrafeCommand(
	TEXT("AS.Test.Strafe"),
	TEXT("AS.Test.Strafe [Period=1] [Speed]: this window's character strafes left and right, turning every Period seconds, at Speed cm/s if given. Type again to stop. Speed is for the listen-server window only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const double Period = Args.Num() > 0 ? FMath::Max(FCString::Atod(*Args[0]), 0.1) : 1.;
		const float Speed = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.f;
		ToggleTicker(StrafeHandle, World, [Period, Speed](APlayerController& PC)
		{
			ACharacter* Character = Cast<ACharacter>(PC.GetPawn());
			if (!Character)
			{
				return;
			}

			if (Speed > 0.f)
			{
				// Full speed within a quarter period, so every strafe reaches it before turning.
				UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
				Movement->MaxWalkSpeed = Speed;
				Movement->MaxAcceleration = 4.f * Speed / static_cast<float>(Period);
			}

			const bool bRight = FMath::FloorToInt64(PC.GetWorld()->GetTimeSeconds() / Period) % 2 == 0;
			Character->AddMovementInput(Character->GetActorRightVector(), bRight ? 1.f : -1.f);
		});
	}));

static FAutoConsoleCommandWithWorldAndArgs GAutoAimCommand(
	TEXT("AS.Test.AutoAim"),
	TEXT("AS.Test.AutoAim [Bone=spine_03]: aim this window's camera at the nearest other living character's bone, where this machine draws it. Type again to stop."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const FName Bone = Args.Num() > 0 ? FName(*Args[0]) : FName(TEXT("spine_03"));
		ToggleTicker(AutoAimHandle, World, [Bone](APlayerController& PC)
		{
			const APawn* Self = PC.GetPawn();
			if (!Self)
			{
				return;
			}

			FVector ViewLocation;
			FRotator ViewRotation;
			PC.GetPlayerViewPoint(ViewLocation, ViewRotation);

			const USkeletalMeshComponent* Target = nullptr;
			double BestDistSq = TNumericLimits<double>::Max();
			for (TActorIterator<ACharacter> It(PC.GetWorld()); It; ++It)
			{
				// Dead characters turn their capsule collision off.
				if (*It == Self || It->GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
				{
					continue;
				}
				const double DistSq = FVector::DistSquared(ViewLocation, It->GetActorLocation());
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					Target = It->GetMesh();
				}
			}

			if (Target)
			{
				PC.SetControlRotation((Target->GetBoneLocation(Bone) - ViewLocation).Rotation());
			}
		});
	}));

static FAutoConsoleCommandWithWorldAndArgs GGiveAmmoCommand(
	TEXT("AS.Test.GiveAmmo"),
	TEXT("AS.Test.GiveAmmo [Amount=9999]: adds ammo to every weapon of every player. Type it in the listen-server window."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World || World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogAS_Weapon, Warning, TEXT("AS.Test.GiveAmmo changes server state: type it in the listen-server window"));
			return;
		}

		const int32 Amount = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 9999;
		for (TActorIterator<ACharacter> It(World); It; ++It)
		{
			const UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(*It);
			const UASInventoryComponent* Inventory = AbilitySystem ? UASInventoryComponent::FindInventoryComponent(AbilitySystem) : nullptr;
			if (!Inventory)
			{
				continue;
			}
			for (int32 Slot = 0; Slot < Inventory->GetCapacity(); ++Slot)
			{
				if (UASWeaponInstance* Weapon = Inventory->GetInstanceAtSlot(Slot))
				{
					Weapon->AddAmmo(Amount);
				}
			}
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GLagCompStressCommand(
	TEXT("AS.Stress.LagComp"),
	TEXT("AS.Stress.LagComp [Characters=100]: times rewound traces for batches of 1 to 10000 rays, single-threaded and with ParallelFor. Blocks the game while it runs. Writes Saved/Profiling/LagCompStress_<Characters>.csv."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		const ACharacter* Player = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
		TArray<FASHitCapsuleDef> Defs;
		if (!Player || !UASLagCompensationSubsystem::BuildCapsuleDefs(*Player->GetMesh(), Defs))
		{
			UE_LOG(LogAS, Warning, TEXT("AS.Stress.LagComp needs a possessed character with a physics asset"));
			return;
		}
		const int32 NumCharacters = Args.Num() > 0 ? FMath::Max(FCString::Atoi(*Args[0]), 1) : 100;

		// Synthetic characters: copies of the player's own posed hitboxes, 3 to 30 m around them.
		// A fixed seed, so every run traces the same scene.
		FRandomStream Random(1234);
		TArray<FASWorldCapsule> Body;
		Body.SetNum(Defs.Num());
		UASLagCompensationSubsystem::PoseCapsules(*Player->GetMesh(), Defs, Body);

		TArray<FASWorldCapsule> Capsules;
		TArray<FASHitboxGroup> Groups;
		TArray<FVector> Centers;
		for (int32 Index = 0; Index < NumCharacters; ++Index)
		{
			const FVector Offset = FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f).Vector() * Random.FRandRange(300.f, 3000.f);
			Centers.Add(Player->GetActorLocation() + Offset);

			const int32 First = Capsules.Num();
			for (FASWorldCapsule Capsule : Body)
			{
				Capsule.A += Offset;
				Capsule.B += Offset;
				Capsules.Add(Capsule);
			}
			Groups.Add(ASHitboxMath::MakeGroup(Capsules, First, Capsules.Num() - First));
		}

		// Rays from the player's eyes at random characters, up to half a metre off centre so some miss.
		FVector ViewLocation;
		FRotator ViewRotation;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

		constexpr int32 MaxRays = 10000;
		TArray<FASShotRay> Rays;
		for (int32 Index = 0; Index < MaxRays; ++Index)
		{
			const FVector Aim = Centers[Random.RandHelper(Centers.Num())] + Random.GetUnitVector() * Random.FRandRange(0.f, 50.f);
			Rays.Add(FASShotRay(ViewLocation, ViewLocation + (Aim - ViewLocation).GetSafeNormal() * 10000.));
		}

		FCollisionQueryParams Params(SCENE_QUERY_STAT(LagCompStress), true, Player);
		Params.bReturnPhysicalMaterial = true;
		TArray<FHitResult> WorldHits;
		WorldHits.SetNum(MaxRays);
		TArray<FASCapsuleHit> CapsuleHits;
		CapsuleHits.SetNum(MaxRays);

		// Median and 95th percentile microseconds of one batch, after three warm-up batches.
		auto Measure = [](int32 NumRays, TFunctionRef<void()> Batch)
		{
			const int32 Iterations = FMath::Clamp(20000 / NumRays, 10, 200);
			TArray<double> Times;
			for (int32 Iteration = -3; Iteration < Iterations; ++Iteration)
			{
				const double Start = FPlatformTime::Seconds();
				Batch();
				if (Iteration >= 0)
				{
					Times.Add((FPlatformTime::Seconds() - Start) * 1e6);
				}
			}
			Times.Sort();
			return TPair<double, double>(Times[Times.Num() / 2], Times[Times.Num() * 95 / 100]);
		};

		// World+Capsules is the game's code path. The capsule-only workloads isolate the hand-written maths,
		// every capsule (Brute) against bounding spheres first (Broad).
		enum class EWork { World, CapsulesBrute, CapsulesBroad };
		const int32 RayCounts[] = { 1, 2, 3, 4, 6, 8, 12, 17, 32, 100, 1000, 10000 };
		const int32 BatchSizes[] = { 1, 2, 4, 8, 16, 32 };
		FString Csv = TEXT("Work,Characters,Rays,Mode,MinRaysPerTask,MedianUs,P95Us\n");

		for (const EWork Work : { EWork::World, EWork::CapsulesBrute, EWork::CapsulesBroad })
		{
			const TCHAR* WorkName = Work == EWork::World ? TEXT("World+Capsules") : Work == EWork::CapsulesBrute ? TEXT("CapsulesBrute") : TEXT("CapsulesBroad");
			for (const int32 NumRays : RayCounts)
			{
				const TArrayView<const FASShotRay> BatchRays = MakeArrayView(Rays).Slice(0, NumRays);
				auto Run = [&](int32 MinRaysPerTask, bool bParallel)
				{
					return Measure(NumRays, [&]
					{
						if (Work == EWork::World)
						{
							UASLagCompensationSubsystem::TraceRays(*World, BatchRays, COLLISION_WEAPON, Params, Capsules, Groups, MinRaysPerTask, bParallel,
								MakeArrayView(WorldHits).Slice(0, NumRays), MakeArrayView(CapsuleHits).Slice(0, NumRays));
							return;
						}
						ParallelFor(TEXT("LagCompStress_Capsules"), NumRays, MinRaysPerTask, [&](int32 Index)
						{
							const FASShotRay& Ray = BatchRays[Index];
							const FVector Dir = (Ray.End - Ray.Start).GetSafeNormal();
							CapsuleHits[Index] = Work == EWork::CapsulesBrute
								? ASHitboxMath::NearestCapsuleHit(Ray.Start, Dir, 10000., Capsules)
								: ASHitboxMath::NearestGroupHit(Ray.Start, Dir, 10000., Groups, Capsules);
						}, bParallel ? EParallelForFlags::None : EParallelForFlags::ForceSingleThread);
					});
				};

				const TPair<double, double> Single = Run(1, false);
				Csv += FString::Printf(TEXT("%s,%d,%d,Single,0,%.1f,%.1f\n"), WorkName, NumCharacters, NumRays, Single.Key, Single.Value);

				double BestMedian = Single.Key;
				int32 BestBatchSize = 0;
				for (const int32 BatchSize : BatchSizes)
				{
					const TPair<double, double> Parallel = Run(BatchSize, true);
					Csv += FString::Printf(TEXT("%s,%d,%d,ParallelFor,%d,%.1f,%.1f\n"), WorkName, NumCharacters, NumRays, BatchSize, Parallel.Key, Parallel.Value);
					if (Parallel.Key < BestMedian)
					{
						BestMedian = Parallel.Key;
						BestBatchSize = BatchSize;
					}
				}

				UE_LOG(LogAS, Display, TEXT("LagComp stress %s, %d characters, %5d rays: single %9.1f us (%.2f us/ray), best %s %9.1f us, x%.2f"),
					WorkName, NumCharacters, NumRays, Single.Key, Single.Key / NumRays,
					BestBatchSize ? *FString::Printf(TEXT("ParallelFor min %2d"), BestBatchSize) : TEXT("single            "),
					BestMedian, Single.Key / BestMedian);
			}
		}

		const FString CsvPath = FPaths::ProfilingDir() / FString::Printf(TEXT("LagCompStress_%d.csv"), NumCharacters);
		FFileHelper::SaveStringToFile(Csv, *CsvPath);
		UE_LOG(LogAS, Display, TEXT("LagComp stress written to %s"), *FPaths::ConvertRelativePathToFull(CsvPath));
	}));

#endif