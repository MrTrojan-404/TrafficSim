// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/TrafficPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/FloatingPawnMovement.h"

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
}

void ATrafficPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Bind raw mouse scroll keys directly to our custom functions
	// This saves you from having to set up Input Actions in the Project Settings!
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ATrafficPlayerController::IncreaseCameraSpeed);
		InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ATrafficPlayerController::DecreaseCameraSpeed);
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ATrafficPlayerController::ToggleMouseCursor);
	}
}

void ATrafficPlayerController::IncreaseCameraSpeed()
{
	if (APawn* ControlledPawn = GetPawn())
	{
		// Grab the default flying movement component
		if (UFloatingPawnMovement* Movement = Cast<UFloatingPawnMovement>(ControlledPawn->GetMovementComponent()))
		{
			// Add 1000 speed per scroll click, capping at a max of 15000
			Movement->MaxSpeed = FMath::Clamp(Movement->MaxSpeed + 1000.0f, 500.0f, 15000.0f);
			
			// Boost acceleration too so the camera doesn't feel floaty or icy at high speeds
			Movement->Acceleration = Movement->MaxSpeed; 
		}
	}
}

void ATrafficPlayerController::DecreaseCameraSpeed()
{
	if (APawn* ControlledPawn = GetPawn())
	{
		if (UFloatingPawnMovement* Movement = Cast<UFloatingPawnMovement>(ControlledPawn->GetMovementComponent()))
		{
			// Subtract 1000 speed per scroll click, with a minimum floor of 500
			Movement->MaxSpeed = FMath::Clamp(Movement->MaxSpeed - 1000.0f, 500.0f, 15000.0f);
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