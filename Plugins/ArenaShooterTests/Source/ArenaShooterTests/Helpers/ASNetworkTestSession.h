#pragma once

#include "CoreMinimal.h"
#include "Commands/TestCommandBuilder.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"

/**
 * A listen server and one client in PIE, on a real map with that map's own game mode: the same idea as Lyra's
 * FShooterTestsNetworkComponent. CQTest's FPIENetworkComponent can't do this, because it always plays a new, empty map.
 */
class FASNetworkTestSession
{
public:
	FASNetworkTestSession(FAutomationTestBase& InTestRunner, FTestCommandBuilder& InCommandBuilder)
		: TestRunner(InTestRunner)
		, CommandBuilder(InCommandBuilder)
	{
	}

	/**
	 * Queues the start: load the map in the editor, play it as a listen server plus one client, wait for both
	 * worlds and the connection, then delay every packet by OneWayLagMs each way. PIE ends when the test tears down.
	 */
	void Start(const FString& MapPackage, int32 OneWayLagMs)
	{
		CommandBuilder
			.Do(TEXT("Stop any running PIE"), []
			{
				if (GEditor->PlayWorld)
				{
					GEditor->EndPlayMap();
				}
			})
			.Then(TEXT("Load the test map"), [MapPackage] { FAutomationEditorCommonUtils::LoadMap(MapPackage); })
			.Then(TEXT("Start a listen server and a client"), [] { StartPlaySession(); })
			.Until(TEXT("Find the server and client worlds"), [this] { return FindWorlds(); }, FTimespan::FromSeconds(60))
			.Until(TEXT("Wait for the client to connect"), [this] { return IsClientConnected(); }, FTimespan::FromSeconds(30))
			.Then(TEXT("Add packet lag"), [this, OneWayLagMs] { SetOneWayLag(OneWayLagMs); })
			.OnTearDown(TEXT("End PIE"), [this]
			{
				ServerWorld = nullptr;
				ClientWorld = nullptr;
				GEditor->RequestEndPlayMap();
			});
	}

	UWorld* GetServerWorld() const { return ServerWorld; }
	UWorld* GetClientWorld() const { return ClientWorld; }

	/** The listen server's own player, on the server. */
	APawn* GetHostOnServer() const { return FindControlledPawn(ServerWorld, true); }

	/** The client's player as the server has it. */
	APawn* GetClientOnServer() const { return FindControlledPawn(ServerWorld, false); }

	/** The client's own player, on the client. */
	APawn* GetClientOnClient() const { return FindControlledPawn(ClientWorld, true); }

	/** The host's player as the client sees it: the one pawn there that isn't the client's own. */
	APawn* GetHostOnClient() const
	{
		if (!ClientWorld)
		{
			return nullptr;
		}
		const APawn* Own = GetClientOnClient();
		for (TActorIterator<APawn> It(ClientWorld); It; ++It)
		{
			if (*It != Own && !It->IsActorBeingDestroyed())
			{
				return *It;
			}
		}
		return nullptr;
	}

private:
	static void StartPlaySession()
	{
		ULevelEditorPlaySettings* PlaySettings = NewObject<ULevelEditorPlaySettings>();
		PlaySettings->SetPlayNetMode(PIE_ListenServer);
		PlaySettings->SetPlayNumberOfClients(2); // the listen server counts as one
		PlaySettings->bLaunchSeparateServer = false;
		PlaySettings->SetRunUnderOneProcess(true);
		PlaySettings->GameGetsMouseControl = false;

		FRequestPlaySessionParams Params;
		Params.WorldType = EPlaySessionWorldType::PlayInEditor;
		Params.EditorPlaySettings = PlaySettings;
		GEditor->RequestPlaySession(Params);
		GEditor->StartQueuedPlaySessionRequest();
	}

	bool FindWorlds()
	{
		ServerWorld = nullptr;
		ClientWorld = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (Context.WorldType != EWorldType::PIE || !World || !World->GetNetDriver())
			{
				continue;
			}
			if (World->GetNetDriver()->IsServer())
			{
				ServerWorld = World;
			}
			else
			{
				ClientWorld = World;
			}
		}
		return ServerWorld && ClientWorld;
	}

	bool IsClientConnected() const
	{
		const UNetDriver* Driver = ServerWorld ? ServerWorld->GetNetDriver() : nullptr;
		return Driver && Driver->ClientConnections.Num() == 1 && Driver->ClientConnections[0]->ViewTarget != nullptr;
	}

	/** PktLag delays only a driver's outgoing packets, so both drivers get it: the round trip is twice LagMs. */
	void SetOneWayLag(int32 LagMs) const
	{
#if DO_ENABLE_NET_TEST
		FPacketSimulationSettings Settings;
		Settings.PktLag = LagMs;
		ServerWorld->GetNetDriver()->SetPacketSimulationSettings(Settings);
		ClientWorld->GetNetDriver()->SetPacketSimulationSettings(Settings);
#endif
	}

	static APawn* FindControlledPawn(UWorld* World, bool bLocal)
	{
		if (!World)
		{
			return nullptr;
		}
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* Controller = It->Get();
			if (Controller && Controller->IsLocalController() == bLocal)
			{
				return Controller->GetPawn();
			}
		}
		return nullptr;
	}

	FAutomationTestBase& TestRunner;
	FTestCommandBuilder& CommandBuilder;
	UWorld* ServerWorld = nullptr;
	UWorld* ClientWorld = nullptr;
};