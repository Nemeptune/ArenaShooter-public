#pragma once

#include "CoreMinimal.h"
#include "ASUIMessages.generated.h"

class APlayerState;

USTRUCT(BlueprintType)
struct FASMatchEndedMessage
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "Match")
	TObjectPtr<APlayerState> Winner = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Match")
	bool bDraw = false;
};
