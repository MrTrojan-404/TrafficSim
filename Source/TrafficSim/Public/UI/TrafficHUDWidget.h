#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TrafficHUDWidget.generated.h"

class UTextBlock;

UCLASS()
class TRAFFICSIM_API UTrafficHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Txt_TotalVehicles;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Txt_ActiveRoadblocks;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Txt_AverageCongestion;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Txt_GameMode;
	
private:
	FTimerHandle StatUpdateTimer;

	UFUNCTION()
	void UpdateStats();

	UFUNCTION()
	void HandleGameModeChanged(ETrafficGameMode NewMode);
};