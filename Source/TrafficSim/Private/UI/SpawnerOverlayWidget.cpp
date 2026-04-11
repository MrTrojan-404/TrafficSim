// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SpawnerOverlayWidget.h"

#include "Component/TrafficSpawnerComponent.h"
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
	// 1. Sever the memory link in the Player Controller FIRST
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		if (PC->ActiveSpawnerWidget == this)
		{
			PC->ActiveSpawnerWidget = nullptr;
		}
	}

	// 2. Restore the correct highlight based on Priority!
	if (TargetNode && TargetNode->SpawnerComponent)
	{
		// PRIORITY 1: Is there an active Rush Hour surge?
		if (TargetNode->SpawnerComponent->QueuedCarsToSpawn > 0)
		{
			TargetNode->SetHighlight(252); 
		}
		// PRIORITY 2: Is it a normal, active spawner?
		else if (TargetNode->SpawnerComponent->bIsActiveSpawner)
		{
			TargetNode->SetHighlight(251); 
		}
		// PRIORITY 3: It's just a normal intersection
		else
		{
			TargetNode->SetHighlight(0); 
		}
	}
    
	// 3. Finally, remove from screen
	RemoveFromParent();
}
void USpawnerOverlayWidget::OnRandomClicked()
{
	if (TargetNode && TargetNode->SpawnerComponent)
	{
		TargetNode->SpawnerComponent->SetAsRandomSpawner();
	}
	CloseUI();
}

void USpawnerOverlayWidget::OnSpecificClicked()
{
	if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetOwningPlayer()))
	{
		PC->StartSelectingDestination(TargetNode);
       
		if (PC->ActiveSpawnerWidget == this)
		{
			PC->ActiveSpawnerWidget = nullptr;
		}
	}
    
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