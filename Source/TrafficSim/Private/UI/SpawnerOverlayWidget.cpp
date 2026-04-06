// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SpawnerOverlayWidget.h"
#include "Components/Button.h"
#include "Controller/TrafficPlayerController.h"
#include "Road/IntersectionNode.h"

void USpawnerOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_RandomDest) Btn_RandomDest->OnClicked.AddDynamic(this, &USpawnerOverlayWidget::OnRandomClicked);
	if (Btn_SpecificDest) Btn_SpecificDest->OnClicked.AddDynamic(this, &USpawnerOverlayWidget::OnSpecificClicked);
	if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &USpawnerOverlayWidget::OnCloseClicked);
}

void USpawnerOverlayWidget::CloseUI()
{
	if (TargetNode)
	{
		TargetNode->SetHighlight(0);
	}
	RemoveFromParent();
}

void USpawnerOverlayWidget::OnRandomClicked()
{
	if (TargetNode)
	{
		TargetNode->SetAsRandomSpawner();
	}
	CloseUI();
}

void USpawnerOverlayWidget::OnSpecificClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		// Tell the controller to wait for the next click!
		PC->StartSelectingDestination(TargetNode);
	}
	
	// We call RemoveFromParent() directly instead of CloseUI() 
	// because we want the Stencil highlight to STAY ON while they pick the destination!
	RemoveFromParent(); 
}

void USpawnerOverlayWidget::OnCloseClicked()
{
	CloseUI();
}

void USpawnerOverlayWidget::SetTargetNode(AIntersectionNode* InNode)
{
	TargetNode = InNode;
}