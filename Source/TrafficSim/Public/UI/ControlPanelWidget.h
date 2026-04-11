#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ControlPanelWidget.generated.h"

class UButton;

UCLASS()
class TRAFFICSIM_API UControlPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	// Traffic Light Controls
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_LightsNormal;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_LightsRed;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_LightsGreen;

	// City Management
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ClearTraffic;
	
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ClearCity;

	// Close Menu
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Close;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ToggleDriveSide;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_SaveLayout;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_LoadLayout;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_LoadDefault;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_TogglePulse;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_TimeDil1;
	
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_TimeDil2;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_TimeDil5;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ScenarioArtery;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ScenarioStadium;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ExportCSV;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_RepairRoads;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_GenerateCity;

	UPROPERTY(meta = (BindWidget))
	class USpinBox* Spin_CitySize;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_PopulateTraffic;

	UPROPERTY(meta = (BindWidget))
	class USlider* Slider_PanSpeed;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ReplayTutorial;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ToggleHeatmap;

	UPROPERTY(meta = (BindWidget))
	USlider* Slider_TimeOfDay;

private:
	// Button bind functions
	UFUNCTION() void OnLightsNormal();
	UFUNCTION() void OnLightsRed();
	UFUNCTION() void OnLightsGreen();
	UFUNCTION() void OnClearCityClicked();
	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnClearTrafficClicked();
	UFUNCTION() void OnToggleDriveSideClicked();
	UFUNCTION() void OnSaveLayoutClicked();
	UFUNCTION() void OnLoadLayoutClicked();
	UFUNCTION() void OnLoadDefaultClicked();
	UFUNCTION() void OnTogglePulseClicked();
	UFUNCTION() void OnTimeDil1Clicked();
	UFUNCTION() void OnTimeDil2Clicked();
	UFUNCTION() void OnTimeDil5Clicked();
	UFUNCTION() void OnScenarioArteryClicked();
	UFUNCTION() void OnScenarioStadiumClicked();
	UFUNCTION() void OnExportCSVClicked();
	UFUNCTION() void OnRepairRoadsClicked();
	UFUNCTION() void OnGenerateCityClicked();
	UFUNCTION() void OnPopulateTrafficClicked();
	UFUNCTION() void OnPanSpeedChanged(float Value);
	UFUNCTION() void OnReplayTutorialClicked();
	UFUNCTION() void OnToggleHeatmapClicked();
	UFUNCTION() void OnTimeOfDayChanged(float Value);

};