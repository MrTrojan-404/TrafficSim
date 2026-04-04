// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/TrafficPlayerController.h"

#include "Blueprint/UserWidget.h"

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