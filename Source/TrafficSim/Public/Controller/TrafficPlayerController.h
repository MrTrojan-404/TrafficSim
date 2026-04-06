// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TrafficPlayerController.generated.h"

struct FInputActionValue;
/**
 * 
 */
UENUM(BlueprintType)
enum class ETrafficGameMode : uint8
{
	Simulate	UMETA(DisplayName = "Simulate Mode"),
	Build		UMETA(DisplayName = "Build Mode")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameModeChanged, ETrafficGameMode, NewMode);

UCLASS()
class TRAFFICSIM_API ATrafficPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATrafficPlayerController();

	// The current state of the game
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
	ETrafficGameMode CurrentGameMode = ETrafficGameMode::Simulate;

	// The node we want to spawn when clicking the ground
	UPROPERTY(EditDefaultsOnly, Category = "Building")
	TSubclassOf<class AIntersectionNode> IntersectionClassToSpawn;
	
	// The blueprint version of our UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> HUDWidgetClass;

	// A pointer to the actual widget once it is created
	UPROPERTY()
	class UUserWidget* HUDWidgetInstance;

	// Memory for the road connection logic
	UPROPERTY()
	class AIntersectionNode* FirstSelectedNode = nullptr;

	// The road we want to spawn
	UPROPERTY(EditDefaultsOnly, Category = "Building")
	TSubclassOf<class ARoadSegment> RoadClassToSpawn;

	UPROPERTY(BlueprintAssignable, Category = "Events | UI")
	FOnGameModeChanged OnGameModeChangedDelegate;
	
protected:
	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* PrimaryClickAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ToggleModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ToggleCursorAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ScrollAction;
	
private:
	void ToggleMouseCursor();
	void ToggleGameMode();
	void OnPrimaryClick();
	void HandleBuildModeClick();

	void OnScroll(const FInputActionValue& Value);
};
