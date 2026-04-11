// Fill out your copyright notice in the Description page of Project Settings.


#include "TrafficSim/Public/Road/IntersectionNode.h"

#include "EngineUtils.h"
#include "TimerManager.h"
#include "Component/TrafficSpawnerComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Road/RoadSegment.h"
#include "Subsystem/TrafficNetworkSubsystem.h"
#include "Vehicle/TrafficSpawner.h"
#include "Vehicle/TrafficVehicle.h"

AIntersectionNode::AIntersectionNode()
{
	PrimaryActorTick.bCanEverTick = false;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(200.0f);
	SphereComponent->SetCollisionProfileName(TEXT("NoCollision"));
	RootComponent = SphereComponent;

	RepresentationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RepresentationMesh"));
	RepresentationMesh->SetupAttachment(RootComponent);
	RepresentationMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	TrafficLightVisual = CreateDefaultSubobject<UPointLightComponent>(TEXT("TrafficLightVisual"));
	TrafficLightVisual->SetupAttachment(RootComponent);
	TrafficLightVisual->SetLightColor(FLinearColor::Green);
	TrafficLightVisual->SetIntensity(5000.0f);

	SpawnerComponent = CreateDefaultSubobject<UTrafficSpawnerComponent>(TEXT("SpawnerComponent"));
}

void AIntersectionNode::BeginPlay()
{
	Super::BeginPlay();

	// GENERATE UNIQUE ID FOR SAVING
	if (!UniqueID.IsValid())
	{
		UniqueID = FGuid::NewGuid();
	}
	OnClicked.AddDynamic(this, &AIntersectionNode::OnIntersectionClicked);
	OnBeginCursorOver.AddDynamic(this, &AIntersectionNode::OnHoverBegin);
	OnEndCursorOver.AddDynamic(this, &AIntersectionNode::OnHoverEnd);

	CycleTrafficLight();
	GetWorld()->GetTimerManager().SetTimer(LightTimerHandle, this, &AIntersectionNode::CycleTrafficLight, 5.0f, true);
}

void AIntersectionNode::ToggleTrafficLights()
{
	bHasTrafficLights = !bHasTrafficLights;

	if (TrafficLightVisual)
	{
		TrafficLightVisual->SetVisibility(bHasTrafficLights);
	}

	// If we just turned the lights OFF, force the visual meshes to look Green/Black 
	// so cars don't look like they are running a red light!
	if (!bHasTrafficLights)
	{
		for (ARoadSegment* Segment : IncomingSegments)
		{
			if (Segment) Segment->SetIntersectionLightColor(this, FLinearColor(0.0f, 0.0f, 0.0f)); 
		}
	}
	else
	{
		// If we turned them back ON, restore the normal light colors
		ApplyLightColors(); 
	}

	UE_LOG(LogTemp, Warning, TEXT("TrafficSim: Intersection %s lights toggled to %d"), *GetName(), bHasTrafficLights);
}

// Route the click directly into your existing logic
void AIntersectionNode::OnIntersectionClicked(AActor* TouchedActor, FKey ButtonPressed)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
    
	if (PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift)))
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficSim: Rush Hour Triggered on this Island!"));
       
		UTrafficNetworkSubsystem* Subsystem = GetWorld()->GetSubsystem<UTrafficNetworkSubsystem>();
		if (!Subsystem) return;

		// 1. Get ONLY the intersections connected to the one we clicked
		TArray<AIntersectionNode*> ConnectedIsland = Subsystem->GetConnectedNetwork(this);

		// 2. Trigger dynamic Intersection Spawners
		for (AIntersectionNode* Node : ConnectedIsland)
		{
			if (IsValid(Node) && Node->SpawnerComponent) 
			{
				Node->SpawnerComponent->TriggerRushHour(10);
			}
		}

		// 3. Trigger hardcoded ATrafficSpawners (if you are still using them)
		TArray<AActor*> Spawners;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATrafficSpawner::StaticClass(), Spawners);
       
		for (AActor* Actor : Spawners)
		{
			ATrafficSpawner* Spawner = Cast<ATrafficSpawner>(Actor);
			// Only trigger if the spawner is physically attached to this specific island
			if (Spawner && ConnectedIsland.Contains(Spawner->StartNode)) 
			{
				Spawner->TriggerRushHour(10); 
			}
		}
	}
	// TOGGLE TRAFFIC LIGHTS (Ctrl + Click)
	else if (PC && (PC->IsInputKeyDown(EKeys::LeftAlt) || PC->IsInputKeyDown(EKeys::RightAlt)))
	{
		ToggleTrafficLights();
	}
	else
	{
		PlayerForceLightChange();
	}
}

void AIntersectionNode::CycleTrafficLight()
{
	if (IncomingSegments.Num() == 0) return;

	GreenLightIndex++;
	if (GreenLightIndex >= IncomingSegments.Num())
	{
		GreenLightIndex = 0;
	}

	CurrentGreenRoad = IncomingSegments[GreenLightIndex];

	// Only physically change the light colors if we are in Normal mode!
	// (If we are overridden, the cycle still tracks silently in the background)
	ApplyLightColors();
}

void AIntersectionNode::SetHighlight(int32 StencilValue)
{
	PermanentStencil = StencilValue;
	ApplyStencil(StencilValue);
}

void AIntersectionNode::ApplyStencil(int32 StencilValue)
{
	TArray<UStaticMeshComponent*> AllMeshes;
	GetComponents<UStaticMeshComponent>(AllMeshes);

	for (UStaticMeshComponent* Mesh : AllMeshes)
	{
		Mesh->SetRenderCustomDepth(StencilValue > 0);
		Mesh->SetCustomDepthStencilValue(StencilValue);
	}
}

void AIntersectionNode::OnHoverBegin(AActor* TouchedActor)
{
	// Only apply the hover color (e.g., 253 for White/Purple) if it isn't already highlighted!
	if (PermanentStencil == 0)
	{
		ApplyStencil(253); 
	}
}

void AIntersectionNode::OnHoverEnd(AActor* TouchedActor)
{
	// Revert back to 0 when the mouse leaves
	if (PermanentStencil == 0)
	{
		ApplyStencil(0);
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

void AIntersectionNode::SetLightState(ELightOverrideState NewState)
{
	LightState = NewState;
	ApplyLightColors();
	UE_LOG(LogTemp, Warning, TEXT("Node %s light state changed to %d"), *GetName(), (uint8)LightState);
}

void AIntersectionNode::ApplyLightColors()
{
	// 1. ALL RED OVERRIDE
	if (LightState == ELightOverrideState::AllRed)
	{
		for (ARoadSegment* Segment : IncomingSegments)
			if (Segment) Segment->SetIntersectionLightColor(this, FLinearColor::Red);
	}
	// 2. ALL GREEN OVERRIDE
	else if (LightState == ELightOverrideState::AllGreen)
	{
		for (ARoadSegment* Segment : IncomingSegments)
			if (Segment) Segment->SetIntersectionLightColor(this, FLinearColor::Green);
	}
	// 3. NORMAL OPERATION
	else
	{
		for (ARoadSegment* Segment : IncomingSegments)
		{
			if (Segment)
			{
				FLinearColor Color = (Segment == CurrentGreenRoad) ? FLinearColor::Green : FLinearColor::Red;
				Segment->SetIntersectionLightColor(this, Color);
			}
		}
	}
}

void AIntersectionNode::DestroyIntersectionSafe()
{
	// We must make a temporary COPY of the arrays. 
	// If we iterate through the original array while DestroyRoadSafe() is modifying it, the game will crash!
	TArray<ARoadSegment*> RoadsToDestroy;
	RoadsToDestroy.Append(IncomingSegments);
	RoadsToDestroy.Append(OutgoingSegments);

	for (ARoadSegment* Road : RoadsToDestroy)
	{
		if (IsValid(Road))
		{
			Road->DestroyRoadSafe();
		}
	}

	// Now that the roads are gone, we can safely destroy ourselves
	Destroy();
}

#if WITH_EDITOR
void AIntersectionNode::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	// When the intersection moves, find all roads in the world...
	for (TActorIterator<ARoadSegment> It(GetWorld()); It; ++It)
	{
		// ...if the road is connected to us...
		if (It->StartNode == this || It->EndNode == this)
		{
			// ...force it to rerun its OnConstruction script!
			It->RerunConstructionScripts();
		}
	}
}
#endif