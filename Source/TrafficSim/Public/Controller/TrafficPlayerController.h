// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Road/IntersectionNode.h"
#include "TrafficPlayerController.generated.h"

class AStaticMeshActor;
struct FInputActionValue;
/**
 * 
 */
UENUM(BlueprintType)
enum class ETrafficGameMode : uint8
{
	Simulate	UMETA(DisplayName = "Simulate Mode"),
	Build		UMETA(DisplayName = "Build Mode"),
	Delete    UMETA(DisplayName = "Delete Mode")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameModeChanged, ETrafficGameMode, NewMode);

UCLASS()
class TRAFFICSIM_API ATrafficPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATrafficPlayerController();

	virtual void Tick(float DeltaSeconds) override;
	
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
	void ActivateGodMode();

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

	// Master Lane Control
	UFUNCTION(BlueprintCallable, Category = "Traffic Control")
	void ToggleCityDriveSide();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Control")
	bool bMasterDriveOnLeft = true;

	// ---> SAVE / LOAD SYSTEM <---
	UFUNCTION(BlueprintCallable, Category = "Traffic Control | Saving")
	void SaveCityLayout();

	UFUNCTION(BlueprintCallable, Category = "Traffic Control | Saving")
	void LoadCityLayout();

	UFUNCTION(BlueprintCallable, Category = "Traffic Control | Saving")
	void LoadDefaultLayout();

	// SIMULATION STATS
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Control | Stats")
	int32 TotalTripsCompleted = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Control | Stats")
	float CumulativeTravelTime = 0.0f;

	void RegisterCompletedTrip(float TripDuration);

	// Accessibility Settings
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settings | Accessibility")
	bool bEnableHeatmapPulse = true;

	UFUNCTION(BlueprintCallable, Category = "Settings | Accessibility")
	void ToggleHeatmapPulse();

	// SIMULATION SPEED
	UFUNCTION(BlueprintCallable, Category = "Traffic Control | Time")
	void SetSimulationSpeed(float SpeedMultiplier);

	// DISASTER SCENARIOS
	UFUNCTION(BlueprintCallable, Category = "Traffic Control | Scenarios")
	void TriggerScenario_ArteryCollapse();

	UFUNCTION(BlueprintCallable, Category = "Traffic Control | Scenarios")
	void TriggerScenario_StadiumEvent();

	// ANALYTICS EXPORT
	UFUNCTION(BlueprintCallable, Category = "Traffic Control | Stats")
	void ExportAnalyticsToCSV();

	UFUNCTION(BlueprintCallable, Category = "Traffic Control | Scenarios")
	void ClearAllRoadblocks();

	//PROCEDURAL GENERATION
	UFUNCTION(BlueprintCallable, Category = "Building | Procedural")
	void GenerateProceduralCity(int32 GridSize);

	// Helper function to handle the deferred spawning logic for algorithmic roads
	void SpawnProceduralRoad(AIntersectionNode* Start, AIntersectionNode* End);

	// TRAFFIC POPULATION
	UFUNCTION(BlueprintCallable, Category = "Traffic Control | Procedural")
	void PopulateCityWithTraffic();

	//TUTORIAL SYSTEM
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial UI")
	TSubclassOf<class UTutorialWidget> TutorialWidgetClass;

	UPROPERTY()
	UTutorialWidget* ActiveTutorialWidget;

	int32 CurrentTutorialStep = 0;
	TArray<FString> TutorialObjectives;

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void StartTutorial();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void AdvanceTutorial(int32 CompletedStepIndex);

	//TUTORIAL TIMER
	FTimerHandle TutorialTimerHandle;
	void HideTutorialWidget();

	UPROPERTY(BlueprintReadWrite, Category = "Traffic Control | Settings")
	bool bMasterHeatmapEnabled = true;

	UFUNCTION(BlueprintCallable, Category = "Traffic Control | Settings")
	void ToggleMasterHeatmap();

	//Track Time of Day
	UPROPERTY(BlueprintReadOnly, Category = "Traffic Control | Settings")
	float CurrentTimeOfDay = 0.0f;
	
	UFUNCTION(BlueprintCallable, Category = "Traffic Control | Settings")
	void SetTimeOfDay(float TimeValue); // 0.0 to 1.0

	// GHOST PREVIEW
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Control | Settings")
	UStaticMesh* IntersectionGhostMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Control | Settings")
	UMaterialInterface* GhostMaterial;

	//Throughput Tracking
	UPROPERTY()
	TArray<float> CompletedTripTimestamps;

	UFUNCTION(BlueprintCallable, Category = "Analytics")
	int32 GetCarsPerMinute();

	// Spawn Tracking
	UPROPERTY()
	TArray<float> SpawnedTimestamps;

	void RegisterSpawnedCar();

	UFUNCTION(BlueprintCallable, Category = "Analytics")
	int32 GetSpawnRate();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lighting")
	bool bIsNightTime = false;
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* ToggleGodCameraAction;
	
	// RANDOM EVENTS (The Game Director)
	UPROPERTY(EditDefaultsOnly, Category = "Events")
	TSubclassOf<class ADynamicObstacle> ObstacleClass;

	FTimerHandle RandomEventTimer;

	UFUNCTION()
	void TriggerRandomEvent();

	UPROPERTY()
	AStaticMeshActor* GhostPreviewActor;
private:
	void HandleDeleteModeClick();
	void ToggleMouseCursor();
	void ToggleGameMode();
	void OnPrimaryClick();
	void HandleBuildModeClick();
	void OnSecondaryClick();
	void OnScroll(const FInputActionValue& Value);
};
