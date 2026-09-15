// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MultiplayerHost.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerHost : public UUserWidget
{
	GENERATED_BODY()
	UPROPERTY(meta = (BindWidget))
	class UButton* HostSessionButton;
	UPROPERTY(meta = (BindWidget))
	class UEditableText* SessionNameEText;
	UPROPERTY(meta = (BindWidget))
	class UComboBoxString* ComboMaxPlayers;
public:

	/*UFUNCTION(BlueprintCallable)
	void HostSetup();*/

protected:
	virtual bool Initialize() override;

	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);

private:

	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
	FString MatchType{ TEXT("FreeForAll") };

	UFUNCTION()
	void HostSessionButtonOnClicked();
};
