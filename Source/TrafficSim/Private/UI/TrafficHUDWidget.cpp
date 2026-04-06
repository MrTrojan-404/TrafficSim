// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TrafficHUDWidget.h"

#include "Components/TextBlock.h"
#include "Controller/TrafficPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Road/RoadSegment.h"
#include "Vehicle/TrafficVehicle.h"


void UTrafficHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
    
	// Update stats every 0.5 seconds instead of every frame to save massive CPU power
	GetWorld()->GetTimerManager().SetTimer(StatUpdateTimer, this, &UTrafficHUDWidget::UpdateStats, 0.5f, true);

	// BIND THE EVENT DELEGATE
	// GetOwningPlayer() safely gets the controller associated with this UI
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		// Bind our local C++ function to the Controller's broadcast!
		PC->OnGameModeChangedDelegate.AddDynamic(this, &UTrafficHUDWidget::HandleGameModeChanged);
        
		// Call it once manually to set the initial text when the game starts
		HandleGameModeChanged(PC->CurrentGameMode);
	}
}

void UTrafficHUDWidget::NativeDestruct()
{
	GetWorld()->GetTimerManager().ClearTimer(StatUpdateTimer);
	Super::NativeDestruct();
}

void UTrafficHUDWidget::UpdateStats()
{
	// 1. Count Total Vehicles
	TArray<AActor*> FoundVehicles;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATrafficVehicle::StaticClass(), FoundVehicles);
    
	if (Txt_TotalVehicles)
	{
		Txt_TotalVehicles->SetText(FText::AsNumber(FoundVehicles.Num()));
	}

	// 2. Count Roadblocks & Congestion
	TArray<AActor*> FoundRoads;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoadSegment::StaticClass(), FoundRoads);

	int32 ActiveRoadblocks = 0;
	float TotalCongestionRatio = 0.0f;

	for (AActor* Actor : FoundRoads)
	{
		ARoadSegment* Road = Cast<ARoadSegment>(Actor);
		if (Road)
		{
			if (Road->bIsBlocked) ActiveRoadblocks++;
          
			if (Road->MaxCapacity > 0)
			{
				TotalCongestionRatio += FMath::Clamp((float)Road->CurrentVehicleCount / (float)Road->MaxCapacity, 0.0f, 1.0f);
			}
		}
	}

	if (Txt_ActiveRoadblocks)
	{
		Txt_ActiveRoadblocks->SetText(FText::AsNumber(ActiveRoadblocks));
	}

	if (Txt_AverageCongestion)
	{
		float AverageCongestion = (FoundRoads.Num() > 0) ? (TotalCongestionRatio / FoundRoads.Num()) : 0.0f;
       
		int32 CongestionPercent = FMath::RoundToInt(AverageCongestion * 100.0f);
		FString PercentString = FString::Printf(TEXT("%d%%"), CongestionPercent);
       
		Txt_AverageCongestion->SetText(FText::FromString(PercentString));
	}
}

void UTrafficHUDWidget::HandleGameModeChanged(ETrafficGameMode NewMode)
{
	if (!Txt_GameMode) return;

	if (NewMode == ETrafficGameMode::Build)
	{
		Txt_GameMode->SetText(FText::FromString(TEXT("BUILD MODE")));
		Txt_GameMode->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.8f, 0.0f, 1.0f))); // Gold
	}
	else if (NewMode == ETrafficGameMode::Delete)
	{
		Txt_GameMode->SetText(FText::FromString(TEXT("DELETE MODE")));
		Txt_GameMode->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f))); // Bright Red
	}
	else
	{
		Txt_GameMode->SetText(FText::FromString(TEXT("SIMULATE MODE")));
		Txt_GameMode->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))); // White
	}
}