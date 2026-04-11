// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/TrafficPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Component/TrafficSpawnerComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "Road/DynamicObstacle.h"
#include "Road/IntersectionNode.h"
#include "Road/RoadSegment.h"
#include "SaveGame/TrafficSaveGame.h"
#include "UI/ControlPanelWidget.h"
#include "UI/SpawnerOverlayWidget.h"
#include "UI/Tutorial/TutorialWidget.h"
#include "Vehicle/TrafficSpawner.h"
#include "Vehicle/TrafficVehicle.h"

ATrafficPlayerController::ATrafficPlayerController()
{
	// Enable the mouse cursor and click events natively!
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ATrafficPlayerController::RegisterCompletedTrip(float TripDuration)
{
	TotalTripsCompleted++;
	CumulativeTravelTime += TripDuration;
}

void ATrafficPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Spawn the UI and add it to the player's screen!
	if (HUDWidgetClass)
	{
		HUDWidgetInstance = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidgetInstance)
		{
			HUDWidgetInstance->AddToViewport();
		}
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Start checking for random events every 15 seconds
	GetWorld()->GetTimerManager().SetTimer(RandomEventTimer, this, &ATrafficPlayerController::TriggerRandomEvent, 15.0f, true);
}

void ATrafficPlayerController::TriggerRandomEvent()
{
	if (!ObstacleClass) return;

	TArray<AActor*> AllRoads;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoadSegment::StaticClass(), AllRoads);

	if (AllRoads.Num() > 0)
	{
		// 1. Pick a random road
		int32 RandomIndex = FMath::RandRange(0, AllRoads.Num() - 1);
		ARoadSegment* ChosenRoad = Cast<ARoadSegment>(AllRoads[RandomIndex]);

		// Don't spawn a cow on a road that is already blocked or too short!
		if (ChosenRoad && !ChosenRoad->bIsBlocked && !ChosenRoad->bHasDynamicObstacle && ChosenRoad->SplineComponent->GetSplineLength() > 1000.0f)
		{
			// 2. Pick a random distance along that road
			float RoadLength = ChosenRoad->SplineComponent->GetSplineLength();
			float RandomDistance = FMath::RandRange(500.0f, RoadLength - 500.0f);

			// 3. Spawn the Cow!
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			
			ADynamicObstacle* NewObstacle = GetWorld()->SpawnActor<ADynamicObstacle>(ObstacleClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

			if (NewObstacle)
			{
				NewObstacle->StartCrossing(ChosenRoad, RandomDistance);
				UE_LOG(LogTemp, Warning, TEXT("RANDOM EVENT: A dynamic obstacle has wandered onto road %s!"), *ChosenRoad->GetName());
			}
		}
	}
}

void ATrafficPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(PrimaryClickAction, ETriggerEvent::Started, this, &ATrafficPlayerController::OnPrimaryClick);
		EnhancedInputComponent->BindAction(ToggleModeAction, ETriggerEvent::Started, this, &ATrafficPlayerController::ToggleGameMode);
		EnhancedInputComponent->BindAction(ToggleCursorAction, ETriggerEvent::Started, this, &ATrafficPlayerController::ToggleMouseCursor);
		EnhancedInputComponent->BindAction(SecondaryClickAction, ETriggerEvent::Started, this, &ATrafficPlayerController::OnSecondaryClick);
		EnhancedInputComponent->BindAction(ScrollAction, ETriggerEvent::Triggered, this, &ATrafficPlayerController::OnScroll);
		EnhancedInputComponent->BindAction(ToggleMenuAction, ETriggerEvent::Started, this, &ATrafficPlayerController::ToggleControlPanel);
		
	}
}

void ATrafficPlayerController::OnScroll(const FInputActionValue& Value)
{
	float ScrollDirection = Value.Get<float>();

	if (APawn* ControlledPawn = GetPawn())
	{
		if (UFloatingPawnMovement* Movement = Cast<UFloatingPawnMovement>(ControlledPawn->GetMovementComponent()))
		{
			// Add or subtract 1000 speed based on scroll direction
			Movement->MaxSpeed = FMath::Clamp(Movement->MaxSpeed + (ScrollDirection * 1000.0f), 500.0f, 15000.0f);
			Movement->Acceleration = Movement->MaxSpeed; 
		}
	}
}
void ATrafficPlayerController::ToggleMouseCursor()
{
	bShowMouseCursor = !bShowMouseCursor;
	bEnableClickEvents = bShowMouseCursor;
	bEnableMouseOverEvents = bShowMouseCursor;

	if (bShowMouseCursor)
	{
		SetInputMode(FInputModeGameAndUI());
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
	AdvanceTutorial(1);
}

void ATrafficPlayerController::ToggleGameMode()
{
	// Clean up highlights if we are leaving Build Mode
	if (FirstSelectedNode)
	{
		FirstSelectedNode->SetHighlight(0);
		FirstSelectedNode = nullptr;
	}

	// Cycle: Simulate -> Build -> Delete -> Simulate
	if (CurrentGameMode == ETrafficGameMode::Simulate)
	{
		CurrentGameMode = ETrafficGameMode::Build;
	}
	else if (CurrentGameMode == ETrafficGameMode::Build)
	{
		CurrentGameMode = ETrafficGameMode::Delete;
	}
	else
	{
		CurrentGameMode = ETrafficGameMode::Simulate;
	}

	OnGameModeChangedDelegate.Broadcast(CurrentGameMode);
	AdvanceTutorial(2);
}

void ATrafficPlayerController::StartSelectingDestination(AIntersectionNode* SpawnerNode)
{
	PendingSpawnerNode = SpawnerNode;
	UE_LOG(LogTemp, Warning, TEXT("Please click a destination node..."));
	// Notice we leave the Stencil Highlight ON here so the player remembers which spawner they are configuring!
}

void ATrafficPlayerController::HandleDeleteModeClick()
{
	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		AActor* HitActor = HitResult.GetActor();

		// Did we click a Road?
		if (ARoadSegment* ClickedRoad = Cast<ARoadSegment>(HitActor))
		{
			ClickedRoad->DestroyRoadSafe();
			UE_LOG(LogTemp, Warning, TEXT("Deleted Road!"));
		}
		// Did we click an Intersection?
		else if (AIntersectionNode* ClickedNode = Cast<AIntersectionNode>(HitActor))
		{
			ClickedNode->DestroyIntersectionSafe();
			UE_LOG(LogTemp, Warning, TEXT("Deleted Intersection and its connected roads!"));
		}
	}
}

void ATrafficPlayerController::OnPrimaryClick()
{
	if (!bShowMouseCursor) return;

	if (CurrentGameMode == ETrafficGameMode::Build)
	{
		HandleBuildModeClick();
	}
	else if (CurrentGameMode == ETrafficGameMode::Delete) 
	{
		HandleDeleteModeClick();
	}
	else
	{
		FHitResult HitResult;
		if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
		{
			AIntersectionNode* ClickedNode = Cast<AIntersectionNode>(HitResult.GetActor());
			if (!ClickedNode) return;

			// 1. Specific Destination Linking
			if (PendingSpawnerNode)
			{
				if (PendingSpawnerNode != ClickedNode)
				{
					if (PendingSpawnerNode->SpawnerComponent)
					{
						PendingSpawnerNode->SpawnerComponent->SetAsSpecificSpawner(ClickedNode);
					}
					PendingSpawnerNode->SetHighlight(0);
					PendingSpawnerNode = nullptr;
				}
				return;
			}
		}
	}
}

void ATrafficPlayerController::OnSecondaryClick()
{
	if (!bShowMouseCursor || CurrentGameMode == ETrafficGameMode::Build) return;

	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		AIntersectionNode* ClickedNode = Cast<AIntersectionNode>(HitResult.GetActor());
		
		// Open the UI on Right Click!
		if (ClickedNode && SpawnerWidgetClass && !PendingSpawnerNode)
		{
			if (ActiveSpawnerWidget)
			{
				ActiveSpawnerWidget->CloseUI(); 
			}

			ActiveSpawnerWidget = CreateWidget<USpawnerOverlayWidget>(this, SpawnerWidgetClass);
			if (ActiveSpawnerWidget)
			{
				ActiveSpawnerWidget->SetTargetNode(ClickedNode);
				ActiveSpawnerWidget->AddToViewport();
				
				ClickedNode->SetHighlight(252);
				AdvanceTutorial(5);
			}
		}
	}
}

void ATrafficPlayerController::HandleBuildModeClick()
{
	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		// SCENARIO 1: Did we click on an existing Intersection?
		AIntersectionNode* ClickedNode = Cast<AIntersectionNode>(HitResult.GetActor());
		if (ClickedNode)
		{
			// If we haven't selected a node yet, save this one as the start!
			if (!FirstSelectedNode)
			{
				FirstSelectedNode = ClickedNode;
				
				// TURN ON HIGHLIGHT
				FirstSelectedNode->SetHighlight(254); //254 - Green
				
				UE_LOG(LogTemp, Warning, TEXT("First Node Selected! Click another to connect."));
				return;
			}
			// If we clicked a SECOND, different node, build the road!
			else if (FirstSelectedNode != ClickedNode)
			{
				if (RoadClassToSpawn)
				{
					// Set the spawn transform to absolute zero
					FTransform SpawnTransform(FRotator::ZeroRotator, FVector::ZeroVector);

					// 1. DEFERRED SPAWN
					ARoadSegment* NewRoad = GetWorld()->SpawnActorDeferred<ARoadSegment>(RoadClassToSpawn, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					
					if (NewRoad)
					{
						// 2. SETUP (Now uses absolute world coordinates)
						NewRoad->SetupConnection(FirstSelectedNode, ClickedNode);

						// 3. FINISH
						NewRoad->FinishSpawning(SpawnTransform);
					}
					// TURN OFF HIGHLIGHT 
					FirstSelectedNode->SetHighlight(0);
					FirstSelectedNode = nullptr; 
					return;
				}
				
				// Clear the memory so the player can build another road
				FirstSelectedNode = nullptr; 
				return; 
			}
		}

		// SCENARIO 2: If we didn't click a node, did we click the Ground? (Your existing logic!)
		if (IntersectionClassToSpawn && HitResult.GetActor() && HitResult.GetActor()->ActorHasTag("Ground"))
		{
			FVector GridPlacementLocation = HitResult.ImpactPoint;
			
			// Snap to a grid
			GridPlacementLocation.X = FMath::RoundToFloat(GridPlacementLocation.X / 500.0f) * 500.0f;
			GridPlacementLocation.Y = FMath::RoundToFloat(GridPlacementLocation.Y / 500.0f) * 500.0f;
			GridPlacementLocation.Z += 10.0f; 

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			GetWorld()->SpawnActor<AIntersectionNode>(IntersectionClassToSpawn, GridPlacementLocation, FRotator::ZeroRotator, SpawnParams);

			if (FirstSelectedNode)
			{
				FirstSelectedNode->SetHighlight(0);
			}			
			// If they clicked the ground, clear out any half-started road connections
			FirstSelectedNode = nullptr;
			
		}
	}
}

void ATrafficPlayerController::ToggleControlPanel()
{
	if (ActiveControlPanel)
	{
		// Menu is open, close it!
		ActiveControlPanel->RemoveFromParent();
		ActiveControlPanel = nullptr;
	}
	else if (ControlPanelClass)
	{
		// Menu is closed, open it!
		ActiveControlPanel = CreateWidget<UControlPanelWidget>(this, ControlPanelClass);
		if (ActiveControlPanel)
		{
			ActiveControlPanel->AddToViewport(100); // Z-order 100 ensures it draws OVER the HUD
		}
	}
}

void ATrafficPlayerController::SetMasterLightState(ELightOverrideState MasterState)
{
	TArray<AActor*> AllNodes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIntersectionNode::StaticClass(), AllNodes);

	for (AActor* Actor : AllNodes)
	{
		AIntersectionNode* Node = Cast<AIntersectionNode>(Actor);
		if (Node)
		{
			Node->SetLightState(MasterState);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("MASTER CONTROL: All lights changed!"));
}

void ATrafficPlayerController::ClearCity()
{
	// 1. Destroy Vehicles first so they don't try to route on deleted roads
	TArray<AActor*> Vehicles;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATrafficVehicle::StaticClass(), Vehicles);
	for (AActor* A : Vehicles) A->Destroy();

	// 2. Destroy Spawners
	TArray<AActor*> Spawners;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATrafficSpawner::StaticClass(), Spawners);
	for (AActor* A : Spawners) A->Destroy();

	// 3. Destroy Roads
	TArray<AActor*> Roads;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoadSegment::StaticClass(), Roads);
	for (AActor* A : Roads) A->Destroy();

	// 4. Destroy Intersections Safely!
	TArray<AActor*> Nodes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIntersectionNode::StaticClass(), Nodes);
	for (AActor* A : Nodes)
	{
		AIntersectionNode* Node = Cast<AIntersectionNode>(A);
		if (Node)
		{
			Node->DestroyIntersectionSafe(); // Use the safe teardown!
		}
	}

	TotalTripsCompleted = 0;
	CumulativeTravelTime = 0.0f;
	
	UE_LOG(LogTemp, Warning, TEXT("MASTER CONTROL: City Cleared!"));
}

void ATrafficPlayerController::ClearTraffic()
{
	// 1. Destroy all vehicles
	TArray<AActor*> Vehicles;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATrafficVehicle::StaticClass(), Vehicles);
	for (AActor* A : Vehicles)
	{
		A->Destroy();
	}

	// 2. Wipe the memory of the roads to prevent "Ghost Gridlock"
	TArray<AActor*> Roads;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoadSegment::StaticClass(), Roads);
	for (AActor* A : Roads)
	{
		ARoadSegment* Road = Cast<ARoadSegment>(A);
		if (Road)
		{
			Road->CurrentVehicleCount = 0;
			Road->VehiclesForward.Empty();
			Road->VehiclesBackward.Empty();

			Road->bIsBlocked = false;
			Road->bDisableHeatmap = false;
			Road->SetHighlight(0);
			if (Road->RoadblockVisual) Road->RoadblockVisual->SetVisibility(false);
			Road->UpdateHeatmap(); // Force the color to reset to black
		}
	}
	TotalTripsCompleted = 0;
	CumulativeTravelTime = 0.0f;

	UE_LOG(LogTemp, Warning, TEXT("MASTER CONTROL: Traffic Cleared!"));
}

void ATrafficPlayerController::ToggleHeatmapPulse()
{
	bEnableHeatmapPulse = !bEnableHeatmapPulse;
	UE_LOG(LogTemp, Warning, TEXT("Settings: Heatmap Pulse set to %d"), bEnableHeatmapPulse);
}

void ATrafficPlayerController::ToggleCityDriveSide()
{
	bMasterDriveOnLeft = !bMasterDriveOnLeft;

	TArray<AActor*> Roads;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoadSegment::StaticClass(), Roads);
	
	for (AActor* A : Roads)
	{
		if (ARoadSegment* Road = Cast<ARoadSegment>(A))
		{
			Road->UpdateDriveSide(bMasterDriveOnLeft);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("MASTER CONTROL: Switched drive side to %s"), bMasterDriveOnLeft ? TEXT("LEFT") : TEXT("RIGHT"));
}

void ATrafficPlayerController::SaveCityLayout()
{
	UTrafficSaveGame* SaveGameInstance = Cast<UTrafficSaveGame>(UGameplayStatics::CreateSaveGameObject(UTrafficSaveGame::StaticClass()));
	if (!SaveGameInstance) return;

	// 1. Save Intersections
	TArray<AActor*> Nodes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIntersectionNode::StaticClass(), Nodes);
	for (AActor* A : Nodes)
	{
		if (AIntersectionNode* Node = Cast<AIntersectionNode>(A))
		{
			FNodeSaveData NodeData;
			NodeData.NodeID = Node->UniqueID;
			NodeData.NodeTransform = Node->GetActorTransform();
			SaveGameInstance->SavedNodes.Add(NodeData);
		}
	}

	// 2. Save Roads
	TArray<AActor*> Roads;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoadSegment::StaticClass(), Roads);
	for (AActor* A : Roads)
	{
		if (ARoadSegment* Road = Cast<ARoadSegment>(A))
		{
			// Only save roads that are fully connected
			if (Road->StartNode && Road->EndNode)
			{
				FRoadSaveData RoadData;
				RoadData.StartNodeID = Road->StartNode->UniqueID;
				RoadData.EndNodeID = Road->EndNode->UniqueID;
				SaveGameInstance->SavedRoads.Add(RoadData);
			}
		}
	}

	// 3. Write to disk
	UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("TrafficSlot1"), 0);
	UE_LOG(LogTemp, Warning, TEXT("MASTER CONTROL: City Layout Saved!"));
}

void ATrafficPlayerController::LoadCityLayout()
{
	if (!UGameplayStatics::DoesSaveGameExist(TEXT("TrafficSlot1"), 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("MASTER CONTROL: No save file found!"));
		return;
	}

	UTrafficSaveGame* LoadGameInstance = Cast<UTrafficSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("TrafficSlot1"), 0));
	if (!LoadGameInstance || !IntersectionClassToSpawn || !RoadClassToSpawn) return;

	// 1. Wipe the current city
	ClearCity();

	// A map to help us quickly look up spawned nodes by their old ID
	TMap<FGuid, AIntersectionNode*> SpawnedNodesMap;

	// 2. Spawn the saved Nodes
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (const FNodeSaveData& NodeData : LoadGameInstance->SavedNodes)
	{
		AIntersectionNode* NewNode = GetWorld()->SpawnActor<AIntersectionNode>(IntersectionClassToSpawn, NodeData.NodeTransform, SpawnParams);
		if (NewNode)
		{
			// Assign the saved ID so the roads can find it
			NewNode->UniqueID = NodeData.NodeID;
			SpawnedNodesMap.Add(NodeData.NodeID, NewNode);
		}
	}

	// 3. Spawn the saved Roads and connect them
	for (const FRoadSaveData& RoadData : LoadGameInstance->SavedRoads)
	{
		if (SpawnedNodesMap.Contains(RoadData.StartNodeID) && SpawnedNodesMap.Contains(RoadData.EndNodeID))
		{
			AIntersectionNode* RetrievedStart = SpawnedNodesMap[RoadData.StartNodeID];
			AIntersectionNode* RetrievedEnd = SpawnedNodesMap[RoadData.EndNodeID];

			FTransform SpawnTransform(FRotator::ZeroRotator, FVector::ZeroVector);
			ARoadSegment* NewRoad = GetWorld()->SpawnActorDeferred<ARoadSegment>(RoadClassToSpawn, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            
			if (NewRoad)
			{
				NewRoad->SetupConnection(RetrievedStart, RetrievedEnd);
				NewRoad->FinishSpawning(SpawnTransform);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("MASTER CONTROL: City Layout Loaded!"));
}

void ATrafficPlayerController::SetSimulationSpeed(float SpeedMultiplier)
{
	// 1.0 is normal speed. 2.0 is double speed. 0.0 is paused!
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), SpeedMultiplier);
	UE_LOG(LogTemp, Warning, TEXT("MASTER CONTROL: Time Dilation set to %f"), SpeedMultiplier);
}

void ATrafficPlayerController::LoadDefaultLayout()
{
	// Reloading the current level instantly restores the editor-placed layout!
	FString CurrentLevelName = GetWorld()->GetName();
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName), false);
}

void ATrafficPlayerController::TriggerScenario_ArteryCollapse()
{
	TArray<AActor*> AllRoads;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoadSegment::StaticClass(), AllRoads);

	int32 RoadsToBlock = 3; // How many roads to destroy
	int32 BlockedCount = 0;

	// Shuffle the array to randomize the targets
	for (int32 i = AllRoads.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		AllRoads.Swap(i, j);
	}

	for (AActor* Actor : AllRoads)
	{
		ARoadSegment* Road = Cast<ARoadSegment>(Actor);
		// Only block active, unblocked roads
		if (Road && !Road->bIsBlocked)
		{
			Road->bIsBlocked = true;
			Road->bDisableHeatmap = true;
			Road->BlockedDistance = FMath::RandRange(500.0f, Road->SplineComponent->GetSplineLength() - 500.0f);
			Road->SetHighlight(250); // 250 - pink
          
			BlockedCount++;
			if (BlockedCount >= RoadsToBlock) break;
		}
	}
	AdvanceTutorial(10);
	
	UE_LOG(LogTemp, Warning, TEXT("SCENARIO: Artery Collapse triggered! %d roads blocked."), BlockedCount);
}

void ATrafficPlayerController::TriggerScenario_StadiumEvent()
{
	TArray<AActor*> Intersections;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIntersectionNode::StaticClass(), Intersections);

	if (Intersections.Num() > 1) // Ensure we have at least 2 nodes!
	{
		int32 RandomIndex = FMath::RandRange(0, Intersections.Num() - 1);
		AIntersectionNode* StadiumNode = Cast<AIntersectionNode>(Intersections[RandomIndex]);

		if (StadiumNode && StadiumNode->SpawnerComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("SCENARIO: Stadium Event at Node %s!"), *StadiumNode->GetName());

			// 1. Pick a completely different node to act as the "Escape Route" destination
			AIntersectionNode* EscapeNode = nullptr;
			while (!EscapeNode || EscapeNode == StadiumNode)
			{
				EscapeNode = Cast<AIntersectionNode>(Intersections[FMath::RandRange(0, Intersections.Num() - 1)]);
			}

			// 2. Tell the Spawner where to send the crowd
			StadiumNode->SpawnerComponent->SetAsSpecificSpawner(EscapeNode);

			// 3. Trigger the wave (I recommend 20-30 cars so they don't spawn inside each other)
			StadiumNode->SpawnerComponent->TriggerRushHour(25);
          
			// 4. Highlight the node so you know where to look!
			StadiumNode->SetHighlight(252); 
		}
	}
}

void ATrafficPlayerController::ExportAnalyticsToCSV()
{
	// 1. Calculate our current metrics
	float AverageTime = (TotalTripsCompleted > 0) ? (CumulativeTravelTime / TotalTripsCompleted) : 0.0f;

	TArray<AActor*> Vehicles;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATrafficVehicle::StaticClass(), Vehicles);
	int32 ActiveCars = Vehicles.Num();

	// 2. Build the CSV String (Commas separate columns, \n separates rows)
	FString CSVContent = TEXT("Traffic Network Analytics Report\n\n");
    
	CSVContent += TEXT("Metric,Value\n");
	CSVContent += FString::Printf(TEXT("Total Trips Completed,%d\n"), TotalTripsCompleted);
	CSVContent += FString::Printf(TEXT("Active Vehicles on Grid,%d\n"), ActiveCars);
	CSVContent += FString::Printf(TEXT("Cumulative Travel Time (Seconds),%f\n"), CumulativeTravelTime);
	CSVContent += FString::Printf(TEXT("Average Travel Time (Seconds),%f\n"), AverageTime);
    
	// Add a timestamp so they know exactly when the test was run
	CSVContent += TEXT("\nReport Generated at: ,") + FDateTime::Now().ToString();

	// 3. Define the save location (YourProjectFolder/Saved/TrafficAnalytics.csv)
	FString FilePath = FPaths::ProjectSavedDir() + TEXT("TrafficAnalytics.csv");

	// 4. Write to disk and notify the user!
	if (FFileHelper::SaveStringToFile(CSVContent, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("ANALYTICS EXPORTED: Successfully saved to %s"), *FilePath);
        
		// Print a massive green message to the screen so the judges see it work
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Green, FString::Printf(TEXT("SUCCESS: Analytics exported to %s"), *FilePath));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ANALYTICS EXPORT FAILED!"));
	}
}

void ATrafficPlayerController::ClearAllRoadblocks()
{
	// 1. REPAIR ROADS
	TArray<AActor*> Roads;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoadSegment::StaticClass(), Roads);
	int32 RepairedCount = 0;

	for (AActor* A : Roads)
	{
		ARoadSegment* Road = Cast<ARoadSegment>(A);
		if (Road && Road->bIsBlocked)
		{
			Road->bIsBlocked = false;
			Road->bDisableHeatmap = false;
			Road->SetHighlight(0);
			if (Road->RoadblockVisual) Road->RoadblockVisual->SetVisibility(false);
			Road->UpdateHeatmap(); 
			RepairedCount++;
		}
	}

	// 2.NEW: CANCEL SURGES & CLEAR HIGHLIGHTS
	TArray<AActor*> Nodes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIntersectionNode::StaticClass(), Nodes);
	for (AActor* A : Nodes)
	{
		AIntersectionNode* Node = Cast<AIntersectionNode>(A);
		if (Node)
		{
			// Clear the purple Stadium highlight or the Procedural Spawner highlight
			Node->SetHighlight(0); 

			// Kill the active queue
			if (Node->SpawnerComponent)
			{
				Node->SpawnerComponent->CancelRushHour();
			}
		}
	}
	AdvanceTutorial(11);
	
	UE_LOG(LogTemp, Warning, TEXT("MASTER CONTROL: Repaired %d roadblocks and canceled active surges!"), RepairedCount);
}

void ATrafficPlayerController::GenerateProceduralCity(int32 GridSize)
{
    // 1. Wipe the slate clean before generating!
    ClearCity();

    // 2. Failsafe: Don't let the user generate a city of size 0 or crash the engine with size 1000!
    GridSize = FMath::Clamp(GridSize, 2, 20); 

    float BaseSpacing = 6000.0f; 
    
    TArray<AIntersectionNode*> GridNodes;
    GridNodes.Init(nullptr, GridSize * GridSize);

    // 2. SPAWN THE NODES
    for (int32 x = 0; x < GridSize; x++)
    {
        for (int32 y = 0; y < GridSize; y++)
        {
            float OffsetX = FMath::RandRange(-800.0f, 800.0f);
            float OffsetY = FMath::RandRange(-800.0f, 800.0f);
            
            FVector SpawnLoc((x * BaseSpacing) + OffsetX, (y * BaseSpacing) + OffsetY, 10.0f);
            
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            AIntersectionNode* NewNode = GetWorld()->SpawnActor<AIntersectionNode>(IntersectionClassToSpawn, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
            
            GridNodes[x + (y * GridSize)] = NewNode;
        }
    }

    // 3. CONNECT THE ROADS
    for (int32 x = 0; x < GridSize; x++)
    {
        for (int32 y = 0; y < GridSize; y++)
        {
            AIntersectionNode* CurrentNode = GridNodes[x + (y * GridSize)];
            if (!CurrentNode) continue;

            // Connect RIGHT
            if (x < GridSize - 1 && FMath::FRand() < 0.85f)
            {
                AIntersectionNode* RightNode = GridNodes[(x + 1) + (y * GridSize)];
                SpawnProceduralRoad(CurrentNode, RightNode);
            }

            // Connect UP
            if (y < GridSize - 1 && FMath::FRand() < 0.85f)
            {
                AIntersectionNode* UpNode = GridNodes[x + ((y + 1) * GridSize)];
                SpawnProceduralRoad(CurrentNode, UpNode);
            }
        }
    }
	AdvanceTutorial(3);    
    UE_LOG(LogTemp, Warning, TEXT("PROCEDURAL GENERATION COMPLETE: %dx%d City generated."), GridSize, GridSize);
}

void ATrafficPlayerController::SpawnProceduralRoad(AIntersectionNode* Start, AIntersectionNode* End)
{
    if (!Start || !End || !RoadClassToSpawn) return;

    FTransform SpawnTransform(FRotator::ZeroRotator, FVector::ZeroVector);
    
    // We use Deferred Spawning so we can run SetupConnection BEFORE the road runs its Construction Script
    ARoadSegment* NewRoad = GetWorld()->SpawnActorDeferred<ARoadSegment>(RoadClassToSpawn, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    
    if (NewRoad)
    {
        NewRoad->SetupConnection(Start, End);
        NewRoad->FinishSpawning(SpawnTransform);
    }
}

void ATrafficPlayerController::PopulateCityWithTraffic()
{
	TArray<AActor*> AllNodes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIntersectionNode::StaticClass(), AllNodes);

	if (AllNodes.Num() == 0) return;

	// We don't want every single node spawning cars, or the city will gridlock instantly.
	// Let's activate roughly 25% of the intersections to act as city "entrances/hubs".
	int32 SpawnersToActivate = FMath::Max(1, AllNodes.Num() / 4); 
	int32 ActivatedCount = 0;

	// Shuffle the array to randomize which nodes get selected
	for (int32 i = AllNodes.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		AllNodes.Swap(i, j);
	}

	// Loop through our newly shuffled array and turn on the spawners
	for (AActor* Actor : AllNodes)
	{
		AIntersectionNode* Node = Cast<AIntersectionNode>(Actor);
		if (Node && Node->SpawnerComponent)
		{
			// This calls the function you already wrote to start the random spawn timer!
			Node->SpawnerComponent->SetAsRandomSpawner();
            
			// Turn the node slightly green/blue visually so the player knows it's a spawner
			Node->SetHighlight(251); 
            
			ActivatedCount++;
			if (ActivatedCount >= SpawnersToActivate) break;
		}
	}
	AdvanceTutorial(4);
	UE_LOG(LogTemp, Warning, TEXT("PROCEDURAL TRAFFIC: Activated %d random spawners across the city!"), ActivatedCount);
}

void ATrafficPlayerController::StartTutorial()
{
	// 1. Write the script
	TutorialObjectives = {
		TEXT("Welcome to the Traffic Analytics Tool. Hold Middle-Mouse or Right-Click to orbit the camera."),
		TEXT("Press [TAB] to lock the cursor for seamless Drone Free-Look mode. Press it again to unlock."),
		TEXT("Press [B] to cycle between Simulate, Build, and Delete modes. Return to Simulate Mode."),
		TEXT("Press [M] to open the Control Panel, set a grid size, and click 'Generate City'."),
		TEXT("The infrastructure is ready. Open the panel and click 'Populate Traffic'."),
		TEXT("Right-click any intersection to open its UI and set a specific traffic destination."),
		TEXT("Hold [SHIFT] and Left-Click any intersection to trigger a localized Rush Hour surge."),
		TEXT("Hold [ALT] and Left-Click an intersection to disable its lights, creating a bypass curve."),
		TEXT("Left-Click any intersection to manually cycle its traffic light."),
		TEXT("Left-Click any road to drop a manual roadblock. Click it again to clear it."),
		TEXT("Let's test grid resilience. Open the panel and click 'Simulate Road Closures'."),
		TEXT("Notice the heatmap reacting to the gridlock. Click 'Repair Infrastructure' to clear it.")
	 };

	CurrentTutorialStep = 0;

	// 2. Spawn the Subtitle UI
	if (TutorialWidgetClass && !ActiveTutorialWidget)
	{
		ActiveTutorialWidget = CreateWidget<UTutorialWidget>(this, TutorialWidgetClass);
		if (ActiveTutorialWidget)
		{
			ActiveTutorialWidget->AddToViewport(0); // Z-order 0 keeps it behind your Control Panel
			ActiveTutorialWidget->UpdateSubtitle(TutorialObjectives[0]);
		}
	}
}

void ATrafficPlayerController::AdvanceTutorial(int32 CompletedStepIndex)
{
	// Only advance if the player completed the EXACT step we are currently waiting for
	if (CurrentTutorialStep == CompletedStepIndex)
	{
		CurrentTutorialStep++;

		if (CurrentTutorialStep < TutorialObjectives.Num())
		{
			if (ActiveTutorialWidget)
			{
				ActiveTutorialWidget->UpdateSubtitle(TutorialObjectives[CurrentTutorialStep]);
			}
		}
		else
		{
			// Tutorial Complete
			if (ActiveTutorialWidget)
			{
				ActiveTutorialWidget->UpdateSubtitle(TEXT("Tutorial Complete. Free Sandbox Mode Activated."));
                
				// Wait 5 seconds, then run the HideTutorialWidget function
				GetWorld()->GetTimerManager().SetTimer(TutorialTimerHandle, this, &ATrafficPlayerController::HideTutorialWidget, 5.0f, false);
			}
		}
	}
}
void ATrafficPlayerController::HideTutorialWidget()
{
	if (ActiveTutorialWidget)
	{
		ActiveTutorialWidget->RemoveFromParent(); // Removes it from the screen
		ActiveTutorialWidget = nullptr;           // Clears the memory pointer
	}
}