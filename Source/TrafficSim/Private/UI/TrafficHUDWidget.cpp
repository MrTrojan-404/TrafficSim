// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TrafficHUDWidget.h"

#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Road/RoadSegment.h"
#include "Vehicle/TrafficVehicle.h"


void UTrafficHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// Update stats every 0.5 seconds instead of every frame to save massive CPU power
	GetWorld()->GetTimerManager().SetTimer(StatUpdateTimer, this, &UTrafficHUDWidget::UpdateStats, 0.5f, true);
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
	
	// Directly update the UI Text Block
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

	// Directly update the Roadblocks Text Block
	if (Txt_ActiveRoadblocks)
	{
		Txt_ActiveRoadblocks->SetText(FText::AsNumber(ActiveRoadblocks));
	}

	// Calculate percentage and update the Congestion Text Block
	if (Txt_AverageCongestion)
	{
		float AverageCongestion = (FoundRoads.Num() > 0) ? (TotalCongestionRatio / FoundRoads.Num()) : 0.0f;
		
		// Convert the 0.0-1.0 float into a clean "XX%" string
		int32 CongestionPercent = FMath::RoundToInt(AverageCongestion * 100.0f);
		FString PercentString = FString::Printf(TEXT("%d%%"), CongestionPercent);
		
		Txt_AverageCongestion->SetText(FText::FromString(PercentString));
	}
}