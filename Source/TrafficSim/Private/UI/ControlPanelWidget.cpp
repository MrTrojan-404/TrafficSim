#include "UI/ControlPanelWidget.h"
#include "Components/Button.h"
#include "Controller/TrafficPlayerController.h"

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
	// Tell the controller we are closing so it can clear its pointer
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
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
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->ExportAnalyticsToCSV();
	}
}

void UControlPanelWidget::OnRepairRoadsClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->ClearAllRoadblocks();
	}
}
