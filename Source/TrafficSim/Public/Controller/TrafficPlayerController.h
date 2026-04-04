// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TrafficPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class TRAFFICSIM_API ATrafficPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATrafficPlayerController();
	
	// The blueprint version of our UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> HUDWidgetClass;

	// A pointer to the actual widget once it is created
	UPROPERTY()
	class UUserWidget* HUDWidgetInstance;

protected:
	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;
	
private:
	void IncreaseCameraSpeed();
	void DecreaseCameraSpeed();
	void ToggleMouseCursor();
};
