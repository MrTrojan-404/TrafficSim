// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/TrafficPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Road/IntersectionNode.h"
#include "Road/RoadSegment.h"

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
		
		EnhancedInputComponent->BindAction(ScrollAction, ETriggerEvent::Triggered, this, &ATrafficPlayerController::OnScroll);
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
			FirstSelectedNode->SetHighlight(false);
			FirstSelectedNode = nullptr;
		}
	}
	else
	{
		CurrentGameMode = ETrafficGameMode::Simulate;
	}

	OnGameModeChangedDelegate.Broadcast(CurrentGameMode);
}

void ATrafficPlayerController::OnPrimaryClick()
{
	// Ignore all clicks if we are flying the camera
	if (!bShowMouseCursor) return;

	if (CurrentGameMode == ETrafficGameMode::Build)
	{
		HandleBuildModeClick();
	}
	else
	{
		// Simulate mode logic (e.g., your Rush Hour spawn)
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
				FirstSelectedNode->SetHighlight(true);
				
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
					FirstSelectedNode->SetHighlight(false);
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
				FirstSelectedNode->SetHighlight(false);
			}			
			// If they clicked the ground, clear out any half-started road connections
			FirstSelectedNode = nullptr;
			
		}
	}
}

