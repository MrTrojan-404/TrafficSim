// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TrafficHUDWidget.h"

#include "HttpModule.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Controller/TrafficPlayerController.h"
#include "Interfaces/IHttpRequest.h"
#include "Kismet/GameplayStatics.h"
#include "Road/RoadSegment.h"
#include "Vehicle/TrafficManager.h"


void UTrafficHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Ensure loading screen is hidden at start
	HideLoadingScreen();
	
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
	int32 ActiveCarCount = 0;
	int32 StoppedVehicles = 0;
    
	ATrafficManager* Manager = Cast<ATrafficManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATrafficManager::StaticClass()));
    
	if (Manager)
	{
		// ONE LOOP: Count active cars and stopped cars directly from the parallel arrays!
		for (int32 i = 0; i < Manager->TotalPoolSize; i++)
		{
			if (Manager->bIsActive[i])
			{
				ActiveCarCount++;
				if (Manager->CurrentSpeed[i] < 10.0f)
				{
					StoppedVehicles++;
				}
			}
		}
	}

	if (Txt_TotalVehicles) Txt_TotalVehicles->SetText(FText::AsNumber(ActiveCarCount));

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
    float AverageCongestion = (FoundRoads.Num() > 0) ? (TotalCongestionRatio / FoundRoads.Num()) : 0.0f;

    if (Txt_AverageCongestion)
    {
       int32 CongestionPercent = FMath::RoundToInt(AverageCongestion * 100.0f);
       FString PercentString = FString::Printf(TEXT("%d%%"), CongestionPercent);
       
       Txt_AverageCongestion->SetText(FText::FromString(PercentString));
    }
    
	// 3. Trip Analytics
	float AverageTime = 0.0f;
	int32 CarsPerMinute = 0;
	int32 SpawnRate = 0;

	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		CarsPerMinute = PC->GetCarsPerMinute();
		SpawnRate = PC->GetSpawnRate();

		// Update the in-game UI
		if (Txt_Throughput)
		{
			FString ThroughputString = FString::Printf(TEXT("%d CPM"), CarsPerMinute);
			Txt_Throughput->SetText(FText::FromString(ThroughputString));
		}

		// Update the Spawn Rate UI
		if (Txt_SpawnRate)
		{
			FString SpawnString = FString::Printf(TEXT("%d In/Min"), SpawnRate);
			Txt_SpawnRate->SetText(FText::FromString(SpawnString));
		}

		if (Txt_CompletedTrips)
		{
			Txt_CompletedTrips->SetText(FText::AsNumber(PC->TotalTripsCompleted));
		}

		if (Txt_AverageTravelTime)
		{
			if (PC->TotalTripsCompleted > 0)
			{
				AverageTime = PC->CumulativeTravelTime / PC->TotalTripsCompleted; 
				FString TimeString = FString::Printf(TEXT("%d sec"), FMath::RoundToInt(AverageTime));
				Txt_AverageTravelTime->SetText(FText::FromString(TimeString));
			}
			else
			{
				Txt_AverageTravelTime->SetText(FText::FromString(TEXT("0 sec")));
			}
		}
	}

    // 4. THE EMISSIONS CALCULATION
    // Base moving cars emit 2.0 units of CO2. Idling cars are highly inefficient and emit 10.0 units.
    float MovingCars = ActiveCarCount - StoppedVehicles;
    float TotalEmissions = (MovingCars * 2.0f) + (StoppedVehicles * 10.0f);

	if (Txt_TotalEmissions)
	{
		// Format it nicely so it says something like "450 Units" or "450 kg CO2"
		FString EmissionString = FString::Printf(TEXT("%d kg CO2"), FMath::RoundToInt(TotalEmissions));
		Txt_TotalEmissions->SetText(FText::FromString(EmissionString));
	}
	
	// TELEMETRY BROADCAST
	FString JsonPayload = FString::Printf(TEXT("{\"active_cars\": %d, \"roadblocks\": %d, \"congestion\": %f, \"average_time\": %f, \"emissions\": %f, \"throughput\": %d, \"spawn_rate\": %d}"), 
		   ActiveCarCount, ActiveRoadblocks, AverageCongestion * 100.0f, AverageTime, TotalEmissions, CarsPerMinute, SpawnRate);

    // Create and send the HTTP Request
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
       // Fire and Forget
    });
    
    Request->SetURL("http://127.0.0.1:5000/telemetry"); 
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(JsonPayload);
    Request->ProcessRequest();
}

void UTrafficHUDWidget::ShowLoadingScreen(FString Message)
{
	if (Txt_LoadingMessage)
	{
		Txt_LoadingMessage->SetText(FText::FromString(Message));
	}
    
	if (Border_LoadingOverlay)
	{
		Border_LoadingOverlay->SetVisibility(ESlateVisibility::Visible);
	}
}

void UTrafficHUDWidget::HideLoadingScreen()
{
	if (Border_LoadingOverlay)
	{
		Border_LoadingOverlay->SetVisibility(ESlateVisibility::Hidden);
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