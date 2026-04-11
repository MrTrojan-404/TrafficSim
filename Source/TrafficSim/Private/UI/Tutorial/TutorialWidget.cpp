// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Tutorial/TutorialWidget.h"
#include "Components/TextBlock.h"

void UTutorialWidget::UpdateSubtitle(const FString& NewText)
{
	if (Text_Subtitle)
	{
		Text_Subtitle->SetText(FText::FromString(NewText));
	}
}