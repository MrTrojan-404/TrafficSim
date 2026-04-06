// Fill out your copyright notice in the Description page of Project Settings.


#include "TrafficSim/Public/Road/IntersectionNode.h"

#include "EngineUtils.h"
#include "TimerManager.h"
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
}

void AIntersectionNode::BeginPlay()
{
	Super::BeginPlay();
	
	OnClicked.AddDynamic(this, &AIntersectionNode::OnIntersectionClicked);
	OnBeginCursorOver.AddDynamic(this, &AIntersectionNode::OnHoverBegin);
	OnEndCursorOver.AddDynamic(this, &AIntersectionNode::OnHoverEnd);

	CycleTrafficLight();
	GetWorld()->GetTimerManager().SetTimer(LightTimerHandle, this, &AIntersectionNode::CycleTrafficLight, 5.0f, true);
}

// Route the click directly into your existing logic
void AIntersectionNode::OnIntersectionClicked(AActor* TouchedActor, FKey ButtonPressed)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	
	// Check if the player is holding Left Shift or Right Shift
	if (PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift)))
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficSim: Rush Hour Triggered!"));
		
		// Find every spawner in the city
		TArray<AActor*> Spawners;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATrafficSpawner::StaticClass(), Spawners);
		
		for (AActor* Actor : Spawners)
		{
			ATrafficSpawner* Spawner = Cast<ATrafficSpawner>(Actor);
			if (Spawner)
			{
				// Tell each spawner to rapidly burst 10 cars!
				Spawner->TriggerRushHour(10); 
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficSim: Intersection clicked! Forcing light change."));
		PlayerForceLightChange();
	}
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

void AIntersectionNode::SetAsRandomSpawner()
{
	bIsActiveSpawner = true;
	bIsRandomDest = true;
	
	// Start the timer loop!
	GetWorld()->GetTimerManager().SetTimer(SpawnerTimerHandle, this, &AIntersectionNode::SpawnVehicleRoutine, SpawnInterval, true);
	UE_LOG(LogTemp, Warning, TEXT("Node %s is now a RANDOM Spawner!"), *GetName());
}

void AIntersectionNode::SetAsSpecificSpawner(AIntersectionNode* DestinationNode)
{
	if (!DestinationNode) return;

	bIsActiveSpawner = true;
	bIsRandomDest = false;
	SpecificDestNode = DestinationNode;

	// Start the timer loop!
	GetWorld()->GetTimerManager().SetTimer(SpawnerTimerHandle, this, &AIntersectionNode::SpawnVehicleRoutine, SpawnInterval, true);
	UE_LOG(LogTemp, Warning, TEXT("Node %s is now a SPECIFIC Spawner aimed at %s!"), *GetName(), *DestinationNode->GetName());
}

void AIntersectionNode::SpawnVehicleRoutine()
{
	if (!VehicleClassToSpawn || !bIsActiveSpawner) return;

	UTrafficNetworkSubsystem* Subsystem = GetWorld()->GetSubsystem<UTrafficNetworkSubsystem>();
	if (!Subsystem) return;

	AIntersectionNode* ChosenDestination = nullptr;
	TArray<ARoadSegment*> FinalPath;

	// 1. DETERMINE DESTINATION FIRST
	if (bIsRandomDest)
	{
		TArray<AActor*> AllNodes;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIntersectionNode::StaticClass(), AllNodes);
		
		if (AllNodes.Num() > 1)
		{
			// Try to find a reachable destination (Max 10 attempts so we don't freeze the game)
			for (int i = 0; i < 10; i++)
			{
				int32 RandomIndex = FMath::RandRange(0, AllNodes.Num() - 1);
				AIntersectionNode* PotentialDest = Cast<AIntersectionNode>(AllNodes[RandomIndex]);
				
				if (PotentialDest && PotentialDest != this)
				{
					// Check if a path actually exists!
					TArray<ARoadSegment*> TestPath = Subsystem->FindPath(this, PotentialDest);
					if (TestPath.Num() > 0)
					{
						ChosenDestination = PotentialDest;
						FinalPath = TestPath;
						break; // Found a valid path! Break the loop.
					}
				}
			}
		}
	}
	else
	{
		ChosenDestination = SpecificDestNode;
		FinalPath = Subsystem->FindPath(this, ChosenDestination);
	}

	// If we couldn't find a valid destination or path, abort this specific spawn tick!
	if (!ChosenDestination || FinalPath.Num() == 0) return;

	// 2. NOW WE ACTUALLY SPAWN THE CAR
	FVector SpawnLocation = GetActorLocation() + FVector(0.0f, 0.0f, 150.0f);
	FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

	ATrafficVehicle* NewVehicle = GetWorld()->SpawnActor<ATrafficVehicle>(VehicleClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);

	if (NewVehicle)
	{
		NewVehicle->DebugEndNode = ChosenDestination;
		NewVehicle->SetRoute(FinalPath);
	}
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