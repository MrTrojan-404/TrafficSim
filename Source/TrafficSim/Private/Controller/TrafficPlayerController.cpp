// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/TrafficPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "Road/IntersectionNode.h"
#include "Road/RoadSegment.h"
#include "UI/ControlPanelWidget.h"
#include "UI/SpawnerOverlayWidget.h"
#include "Vehicle/TrafficSpawner.h"
#include "Vehicle/TrafficVehicle.h"

ATrafficPlayerController::ATrafficPlayerController()
{
	// Enable the mouse cursor and click events natively!
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
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
}

void ATrafficPlayerController::ToggleGameMode()
{
	if (CurrentGameMode == ETrafficGameMode::Simulate)
	{
		CurrentGameMode = ETrafficGameMode::Build;
		
		// If we switch modes while a node is selected, turn off its highlight!
		if (FirstSelectedNode)
		{
			FirstSelectedNode->SetHighlight(0); // 0 - no color
			FirstSelectedNode = nullptr;
		}
	}
	else
	{
		CurrentGameMode = ETrafficGameMode::Simulate;
	}

	OnGameModeChangedDelegate.Broadcast(CurrentGameMode);
}

void ATrafficPlayerController::StartSelectingDestination(AIntersectionNode* SpawnerNode)
{
	PendingSpawnerNode = SpawnerNode;
	UE_LOG(LogTemp, Warning, TEXT("Please click a destination node..."));
	// Notice we leave the Stencil Highlight ON here so the player remembers which spawner they are configuring!
}

void ATrafficPlayerController::OnPrimaryClick()
{
	if (!bShowMouseCursor) return;

	if (CurrentGameMode == ETrafficGameMode::Build)
	{
		HandleBuildModeClick();
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
					PendingSpawnerNode->SetAsSpecificSpawner(ClickedNode);
					PendingSpawnerNode->SetHighlight(0);
					PendingSpawnerNode = nullptr;
				}
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

	// 4. Destroy Intersections
	TArray<AActor*> Nodes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIntersectionNode::StaticClass(), Nodes);
	for (AActor* A : Nodes) A->Destroy();

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
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("MASTER CONTROL: Traffic Cleared!"));
}