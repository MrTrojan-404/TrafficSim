// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Road/IntersectionNode.h"
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

	// The Spawner UI Class
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class USpawnerOverlayWidget> SpawnerWidgetClass;
	
	// A reference to the currently open widget so we don't spawn 50 of them
	UPROPERTY()
	USpawnerOverlayWidget* ActiveSpawnerWidget;

	// Called by the UI when the user clicks "Specific"
	void StartSelectingDestination(class AIntersectionNode* SpawnerNode);

	// MASTER MENU SETUP
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UControlPanelWidget> ControlPanelClass;

	UPROPERTY()
	UControlPanelWidget* ActiveControlPanel;

	// Input action to open the menu (e.g., 'M' or 'Escape')
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* ToggleMenuAction;

	// Core Functions
	UFUNCTION(BlueprintCallable, Category = "Traffic Control")
	void SetMasterLightState(ELightOverrideState MasterState);

	UFUNCTION(BlueprintCallable, Category = "Traffic Control")
	void ClearCity();

	UFUNCTION(BlueprintCallable, Category = "Traffic Control")
	void ClearTraffic();
	
	void ToggleControlPanel();
	
protected:
	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	// The node currently waiting for a destination
	UPROPERTY()
	AIntersectionNode* PendingSpawnerNode = nullptr;
	
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SecondaryClickAction;
	
private:
	void ToggleMouseCursor();
	void ToggleGameMode();
	void OnPrimaryClick();
	void HandleBuildModeClick();
	void OnSecondaryClick();
	void OnScroll(const FInputActionValue& Value);
};
