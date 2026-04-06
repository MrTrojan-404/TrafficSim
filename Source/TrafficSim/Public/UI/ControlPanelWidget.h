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
};