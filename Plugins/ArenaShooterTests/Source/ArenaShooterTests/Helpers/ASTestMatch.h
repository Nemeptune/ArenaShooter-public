#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"

/**
 * Turns the match warmup off for one test. The editor's map loader and the network session both wait for
 * the match to start, and a full warmup outlasts them.
 */
class FASWarmupSkip
{
public:
	/** False if the game has no AS.Match.WarmupOverride. */
	bool Begin()
	{
		Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("AS.Match.WarmupOverride"));
		if (!Variable)
		{
			return false;
		}
		Previous = Variable->GetString();
		Variable->Set(TEXT("0"), ECVF_SetByConsole);
		return true;
	}

	void End()
	{
		if (Variable)
		{
			Variable->Set(*Previous, ECVF_SetByConsole);
			Variable = nullptr;
		}
	}

private:
	IConsoleVariable* Variable = nullptr;
	FString Previous;
};

namespace ASTestMatch
{
	/** Harmless engine warnings a running match logs. Tests that play one ignore them, so real warnings stand out. */
	inline void ExpectEngineNoise(FAutomationTestBase& Test)
	{
		// Pooled MetaSounds, whenever a weapon sound plays (see the Sep 21 log triage).
		Test.AddExpectedMessage(TEXT("Array Random Get: Graph Hierarchy environment variable"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, -1, false);
	}
}