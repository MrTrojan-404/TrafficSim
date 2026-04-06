#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpawnerOverlayWidget.generated.h"

class UButton;
class AIntersectionNode;

UCLASS()
class TRAFFICSIM_API USpawnerOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// The C++ function we will call to tell the UI which node we clicked
	UFUNCTION(BlueprintCallable, Category = "Traffic UI")
	void SetTargetNode(AIntersectionNode* InNode);

	void CloseUI();
	
protected:
	// The node this UI is currently modifying
	UPROPERTY(BlueprintReadOnly, Category = "Traffic UI")
	AIntersectionNode* TargetNode;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_RandomDest;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_SpecificDest;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Close;

private:
	UFUNCTION()
	void OnRandomClicked();

	UFUNCTION()
	void OnSpecificClicked();

	UFUNCTION()
	void OnCloseClicked();
};