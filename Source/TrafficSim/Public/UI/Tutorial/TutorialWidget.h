#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialWidget.generated.h"

UCLASS()
class TRAFFICSIM_API UTutorialWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// This exact name must match the Text Block in your Unreal Editor
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_Subtitle;

	UFUNCTION(BlueprintCallable, Category = "Tutorial UI")
	void UpdateSubtitle(const FString& NewText);
};