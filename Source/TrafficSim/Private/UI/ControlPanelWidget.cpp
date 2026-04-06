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
