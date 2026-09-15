// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "ASHostViewModel.generated.h"

class UMultiplayerSessionsSubsystem;
class APlayerController;

USTRUCT(BlueprintType)
struct FASMapOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> Map;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Preview;
};

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASHostViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Grabs the sessions subsystem and binds the create-complete delegate. */
	UFUNCTION(BlueprintCallable, Category = "Host")
	void Initialize(APlayerController* OwningPC);

	UFUNCTION(BlueprintCallable, Category = "Host")
	void InitMaps();

	UFUNCTION(BlueprintCallable, Category = "Host")
	void InitMapsFrom(const TArray<FASMapOption>& InMaps);

	UFUNCTION(BlueprintCallable, Category = "Host")
	void SetSelectedMapIndex(int32 Index);

	UFUNCTION(BlueprintPure, Category = "Host")
	const TArray<FText>& GetMapLabels() const;

	UFUNCTION(BlueprintCallable, Category = "Host")
	void HostGame(const FText& SessionName);

	UPROPERTY(EditAnywhere, Category = "Host")
	TArray<FASMapOption> MapOptions;
	
	UPROPERTY(BlueprintReadWrite, Category = "Host")
	FString MatchType = TEXT("FreeForAll");
	
	UPROPERTY(BlueprintReadWrite, Category = "Host")
	int32 NumConnections = 2;

	void SetStatusText(const FText& Value);
	void SetIsBusy(bool Value);

protected:
	virtual void BeginDestroy() override;
	
private:
	UFUNCTION()
	void HandleCreateComplete(bool bWasSuccessful);

	void RebuildMapLabels();
	void UpdatePreview();
	void SetPreviewTexture(UTexture2D* Texture);

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess))
	FText StatusText;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess))
	bool bIsBusy = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess))
	int32 SelectedMapIndex = 0;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess))
	TObjectPtr<UTexture2D> PreviewTexture;
	
	UPROPERTY()
	TArray<FText> MapLabels;
	
	TWeakObjectPtr<UMultiplayerSessionsSubsystem> Sessions;
};
