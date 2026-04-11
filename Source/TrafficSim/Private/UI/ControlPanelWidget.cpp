#include "UI/ControlPanelWidget.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/SpinBox.h"
#include "Controller/TrafficPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Pawn/RTSCameraPawn.h"
#include "SaveGame/TrafficSaveGame.h"

void UControlPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_ClearTraffic) Btn_ClearTraffic->OnClicked.AddDynamic(this, &UControlPanelWidget::OnClearTrafficClicked);
	if (Btn_LightsNormal) Btn_LightsNormal->OnClicked.AddDynamic(this, &UControlPanelWidget::OnLightsNormal);
	if (Btn_LightsRed) Btn_LightsRed->OnClicked.AddDynamic(this, &UControlPanelWidget::OnLightsRed);
	if (Btn_LightsGreen) Btn_LightsGreen->OnClicked.AddDynamic(this, &UControlPanelWidget::OnLightsGreen);
	if (Btn_ClearCity) Btn_ClearCity->OnClicked.AddDynamic(this, &UControlPanelWidget::OnClearCityClicked);
	if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UControlPanelWidget::OnCloseClicked);
	if (Btn_ToggleDriveSide) Btn_ToggleDriveSide->OnClicked.AddDynamic(this, &UControlPanelWidget::OnToggleDriveSideClicked);
	if (Btn_SaveLayout) Btn_SaveLayout->OnClicked.AddDynamic(this, &UControlPanelWidget::OnSaveLayoutClicked);
	if (Btn_LoadLayout) Btn_LoadLayout->OnClicked.AddDynamic(this, &UControlPanelWidget::OnLoadLayoutClicked);
	if (Btn_LoadDefault) Btn_LoadDefault->OnClicked.AddDynamic(this, &UControlPanelWidget::OnLoadDefaultClicked);
	if (Btn_TogglePulse) Btn_TogglePulse->OnClicked.AddDynamic(this, &UControlPanelWidget::OnTogglePulseClicked);
	if (Btn_TimeDil1) Btn_TimeDil1->OnClicked.AddDynamic(this, &UControlPanelWidget::OnTimeDil1Clicked);
	if (Btn_TimeDil2) Btn_TimeDil2->OnClicked.AddDynamic(this, &UControlPanelWidget::OnTimeDil2Clicked);
	if (Btn_TimeDil5) Btn_TimeDil5->OnClicked.AddDynamic(this, &UControlPanelWidget::OnTimeDil5Clicked);
	if (Btn_ScenarioArtery) Btn_ScenarioArtery->OnClicked.AddDynamic(this, &UControlPanelWidget::OnScenarioArteryClicked);
	if (Btn_ScenarioStadium) Btn_ScenarioStadium->OnClicked.AddDynamic(this, &UControlPanelWidget::OnScenarioStadiumClicked);
	if (Btn_ExportCSV) Btn_ExportCSV->OnClicked.AddDynamic(this, &UControlPanelWidget::OnExportCSVClicked);
	if (Btn_RepairRoads) Btn_RepairRoads->OnClicked.AddDynamic(this, &UControlPanelWidget::OnRepairRoadsClicked);
	if (Btn_GenerateCity) Btn_GenerateCity->OnClicked.AddDynamic(this, &UControlPanelWidget::OnGenerateCityClicked);
	if (Btn_PopulateTraffic) Btn_PopulateTraffic->OnClicked.AddDynamic(this, &UControlPanelWidget::OnPopulateTrafficClicked);
	if (Slider_PanSpeed) Slider_PanSpeed->OnValueChanged.AddDynamic(this, &UControlPanelWidget::OnPanSpeedChanged);
	if (Btn_ReplayTutorial) Btn_ReplayTutorial->OnClicked.AddDynamic(this, &UControlPanelWidget::OnReplayTutorialClicked);
	if (Btn_ToggleHeatmap) Btn_ToggleHeatmap->OnClicked.AddDynamic(this, &UControlPanelWidget::OnToggleHeatmapClicked);
	if (Slider_TimeOfDay) Slider_TimeOfDay->OnValueChanged.AddDynamic(this, &UControlPanelWidget::OnTimeOfDayChanged);

	// Sync sliders to the actual engine values
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		if (Slider_TimeOfDay)
		{
			Slider_TimeOfDay->SetValue(PC->CurrentTimeOfDay);
		}

		if (ARTSCameraPawn* DroneCam = Cast<ARTSCameraPawn>(PC->GetPawn()))
		{
			if (Slider_PanSpeed)
			{
				Slider_PanSpeed->SetValue(DroneCam->GetPanSpeed());
			}
		}
	}
}

void UControlPanelWidget::OnToggleDriveSideClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->ToggleCityDriveSide();
	}
	
}
void UControlPanelWidget::OnLightsNormal()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->SetMasterLightState(ELightOverrideState::Normal);
	}
}

void UControlPanelWidget::OnLightsRed()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->SetMasterLightState(ELightOverrideState::AllRed);
	}
}

void UControlPanelWidget::OnLightsGreen()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->SetMasterLightState(ELightOverrideState::AllGreen);
	}
}

void UControlPanelWidget::OnClearCityClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->ClearCity();
	}
}

void UControlPanelWidget::OnCloseClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		// 1. SAVE SETTINGS TO DISK BEFORE CLOSING
		UTrafficSaveGame* SaveSettings = Cast<UTrafficSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("TrafficSettings"), 0));
		if (!SaveSettings) SaveSettings = Cast<UTrafficSaveGame>(UGameplayStatics::CreateSaveGameObject(UTrafficSaveGame::StaticClass()));

		SaveSettings->SavedTimeOfDay = PC->CurrentTimeOfDay;

		if (ARTSCameraPawn* DroneCam = Cast<ARTSCameraPawn>(PC->GetPawn()))
		{
			SaveSettings->SavedPanSpeed = DroneCam->GetPanSpeed();
		}

		UGameplayStatics::SaveGameToSlot(SaveSettings, TEXT("TrafficSettings"), 0);
		UE_LOG(LogTemp, Warning, TEXT("SETTINGS SAVED: Preferences successfully written to disk."));

		// 2. Close the UI
		PC->ToggleControlPanel(); 
	}
}

void UControlPanelWidget::OnClearTrafficClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->ClearTraffic();
	}
}
void UControlPanelWidget::OnSaveLayoutClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer())) PC->SaveCityLayout();
}

void UControlPanelWidget::OnLoadLayoutClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->LoadCityLayout();
		OnCloseClicked(); // Close menu to see the loaded city!
	}
}

void UControlPanelWidget::OnLoadDefaultClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer())) PC->LoadDefaultLayout();
}

void UControlPanelWidget::OnTogglePulseClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->ToggleHeatmapPulse();
	}
}

void UControlPanelWidget::OnTimeDil1Clicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->SetSimulationSpeed(1);
	}
}

void UControlPanelWidget::OnTimeDil2Clicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->SetSimulationSpeed(2);
	}
}

void UControlPanelWidget::OnTimeDil5Clicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->SetSimulationSpeed(5);
	}
}

void UControlPanelWidget::OnScenarioArteryClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->TriggerScenario_ArteryCollapse();
		// Optional: Close the menu automatically so you can watch the chaos!
		OnCloseClicked(); 
	}
}

void UControlPanelWidget::OnScenarioStadiumClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->TriggerScenario_StadiumEvent();
		OnCloseClicked(); 
	}
}
void UControlPanelWidget::OnExportCSVClicked()
{
	// Instantly opens the default web browser to your Python server
	FPlatformProcess::LaunchURL(TEXT("http://127.0.0.1:5000"), nullptr, nullptr);
}

void UControlPanelWidget::OnRepairRoadsClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->ClearAllRoadblocks();
	}
}

void UControlPanelWidget::OnGenerateCityClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		// Default to a 4x4 city if the Spin Box isn't hooked up properly in Blueprints
		int32 Size = Spin_CitySize ? (int32)Spin_CitySize->GetValue() : 4; 
        
		PC->GenerateProceduralCity(Size);
		OnCloseClicked();
	}
}

void UControlPanelWidget::OnPopulateTrafficClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->PopulateCityWithTraffic();
		OnCloseClicked();
	}
}

void UControlPanelWidget::OnPanSpeedChanged(float Value)
{
	// Grab the physical pawn the player is currently possessing
	if (ARTSCameraPawn* DroneCam = Cast<ARTSCameraPawn>(GetOwningPlayerPawn()))
	{
		DroneCam->SetPanSpeed(Value);
	}
}
void UControlPanelWidget::OnReplayTutorialClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		// Cancel any active tutorial to reset the UI, then start it fresh
		PC->HideTutorialWidget(); 
		PC->StartTutorial();
		OnCloseClicked();
	}
}

void UControlPanelWidget::OnToggleHeatmapClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->ToggleMasterHeatmap();
	}
}
void UControlPanelWidget::OnTimeOfDayChanged(float Value)
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->SetTimeOfDay(Value);
	}
}