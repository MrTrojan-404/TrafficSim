// Fill out your copyright notice in the Description page of Project Settings.


#include "TrafficSim/Public/Road/IntersectionNode.h"
#include "TimerManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Road/RoadSegment.h"

AIntersectionNode::AIntersectionNode()
{
	PrimaryActorTick.bCanEverTick = false;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(200.0f);
	SphereComponent->SetCollisionProfileName(TEXT("NoCollision"));
	RootComponent = SphereComponent;

	// Create the clickable mesh
	ClickableMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClickableMesh"));
	ClickableMesh->SetupAttachment(RootComponent);
	// Ensure it blocks visibility so the mouse click can actually hit it
	ClickableMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	TrafficLightVisual = CreateDefaultSubobject<UPointLightComponent>(TEXT("TrafficLightVisual"));
	TrafficLightVisual->SetupAttachment(RootComponent);
	TrafficLightVisual->SetLightColor(FLinearColor::Green);
	TrafficLightVisual->SetIntensity(5000.0f);
}

void AIntersectionNode::BeginPlay()
{
	Super::BeginPlay();
    
	// Bind the native click event to our function
	if (ClickableMesh)
	{
		ClickableMesh->OnClicked.AddDynamic(this, &AIntersectionNode::OnIntersectionClicked);
	}

	CycleTrafficLight();

	GetWorld()->GetTimerManager().SetTimer(LightTimerHandle, this, &AIntersectionNode::CycleTrafficLight, 5.0f, true);
}

// Route the click directly into your existing logic
void AIntersectionNode::OnIntersectionClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	UE_LOG(LogTemp, Warning, TEXT("TrafficSim: Intersection clicked! Forcing light change."));
	PlayerForceLightChange();
}

void AIntersectionNode::CycleTrafficLight()
{
	if (IncomingSegments.Num() == 0) return;

	// 1. Turn ALL incoming roads to RED first
	for (ARoadSegment* Segment : IncomingSegments)
	{
		if (Segment)
		{
			Segment->SetIntersectionLightColor(this, FLinearColor::Red);
		}
	}

	// 2. Increment the cycle index
	GreenLightIndex++;
	if (GreenLightIndex >= IncomingSegments.Num())
	{
		GreenLightIndex = 0;
	}

	// 3. Turn the newly active road to GREEN
	CurrentGreenRoad = IncomingSegments[GreenLightIndex];
	if (CurrentGreenRoad)
	{
		CurrentGreenRoad->SetIntersectionLightColor(this, FLinearColor::Green);
	}
}

void AIntersectionNode::PlayerForceLightChange()
{
	// Clear the existing automatic timer
	GetWorld()->GetTimerManager().ClearTimer(LightTimerHandle);

	// Force the light to change immediately
	CycleTrafficLight();

	// Restart the automatic timer from 0
	GetWorld()->GetTimerManager().SetTimer(LightTimerHandle, this, &AIntersectionNode::CycleTrafficLight, 5.0f, true);
}