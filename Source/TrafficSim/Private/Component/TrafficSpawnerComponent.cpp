#include "Component/TrafficSpawnerComponent.h"
#include "Road/IntersectionNode.h"
#include "Subsystem/TrafficNetworkSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Road/RoadSegment.h"
#include "Vehicle/TrafficManager.h"

UTrafficSpawnerComponent::UTrafficSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTrafficSpawnerComponent::SetAsRandomSpawner()
{
	bIsActiveSpawner = true;
	bIsRandomDest = true;
	
	GetWorld()->GetTimerManager().SetTimer(SpawnerTimerHandle, this, &UTrafficSpawnerComponent::SpawnVehicleRoutine, SpawnInterval, true);
	UE_LOG(LogTemp, Warning, TEXT("Component on %s is now a RANDOM Spawner!"), *GetOwner()->GetName());
}

void UTrafficSpawnerComponent::SetAsSpecificSpawner(AIntersectionNode* DestinationNode)
{
	if (!DestinationNode) return;

	bIsActiveSpawner = true;
	bIsRandomDest = false;
	SpecificDestNode = DestinationNode;

	GetWorld()->GetTimerManager().SetTimer(SpawnerTimerHandle, this, &UTrafficSpawnerComponent::SpawnVehicleRoutine, SpawnInterval, true);
	UE_LOG(LogTemp, Warning, TEXT("Component on %s is now a SPECIFIC Spawner aimed at %s!"), *GetOwner()->GetName(), *DestinationNode->GetName());
}

void UTrafficSpawnerComponent::AttemptSpawnFromQueue()
{
    // 1. Are we out of cars? Turn off the timer.
    if (QueuedCarsToSpawn <= 0)
    {
        GetWorld()->GetTimerManager().ClearTimer(SpawnQueueTimer);
        return;
    }

    AIntersectionNode* MyNode = Cast<AIntersectionNode>(GetOwner());
    if (!MyNode) return;

    // ---> CONDITION 1: Is the Master Light Red? <---
    if (MyNode->LightState == ELightOverrideState::AllRed)
    {
        return; // Light is red, hold the cars in the stadium!
    }
	
    UTrafficNetworkSubsystem* Subsystem = GetWorld()->GetSubsystem<UTrafficNetworkSubsystem>();
    if (!Subsystem) return;

    AIntersectionNode* ChosenDestination = nullptr;
    TArray<ARoadSegment*> FinalPath;

    // 2. PATHFINDING FIRST (Find out where this specific car wants to go)
    if (bIsRandomDest)
    {
        TArray<AActor*> AllNodes;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIntersectionNode::StaticClass(), AllNodes);

        if (AllNodes.Num() > 1)
        {
            for (int i = 0; i < 10; i++)
            {
                int32 RandomIndex = FMath::RandRange(0, AllNodes.Num() - 1);
                AIntersectionNode* PotentialDest = Cast<AIntersectionNode>(AllNodes[RandomIndex]);

                if (PotentialDest && PotentialDest != MyNode)
                {
                    TArray<ARoadSegment*> TestPath = Subsystem->FindPath(MyNode, PotentialDest);
                    if (TestPath.Num() > 0)
                    {
                        ChosenDestination = PotentialDest;
                        FinalPath = TestPath;
                        break; 
                    }
                }
            }
        }
    }
    else
    {
        ChosenDestination = SpecificDestNode;
        if (ChosenDestination)
        {
            FinalPath = Subsystem->FindPath(MyNode, ChosenDestination);
        }
    }

    // Failsafe: If no path is found, discard the car to prevent an infinite queue lock
    if (!ChosenDestination || FinalPath.Num() == 0)
    {
        QueuedCarsToSpawn--;
        return;
    }

	// 3. TARGET ROAD CHECK
	ARoadSegment* TargetRoad = FinalPath[0]; 

	if (TargetRoad->CurrentVehicleCount >= TargetRoad->MaxCapacity || TargetRoad->bIsBlocked)
	{
		return; 
	}

	// PHYSICAL CLEARANCE CHECK (Using pure indices)
	ATrafficManager* Manager = Cast<ATrafficManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATrafficManager::StaticClass()));
	if (!Manager) return;

	bool bIsForward = (MyNode == TargetRoad->StartNode);
	TArray<int32>& LaneToCheck = bIsForward ? TargetRoad->VehiclesForwardIndices : TargetRoad->VehiclesBackwardIndices;
	float SpawnDistance = bIsForward ? 0.0f : TargetRoad->SplineComponent->GetSplineLength();

	for (int32 CarIndex : LaneToCheck)
	{
		if (Manager->bIsActive[CarIndex])
		{
			if (FMath::Abs(Manager->DistanceAlongSpline[CarIndex] - SpawnDistance) < 500.0f)
			{
				return; // Wait for the car to pull forward!
			}
		}
	}
    
	// SPAWN THE CAR (Using the correct variable names)
	Manager->SpawnCar(FinalPath, ChosenDestination);
	QueuedCarsToSpawn--;
}

void UTrafficSpawnerComponent::SpawnVehicleRoutine()
{
	if (!bIsActiveSpawner) return;
	
	AIntersectionNode* OwningNode = Cast<AIntersectionNode>(GetOwner());
	if (!OwningNode) return; // Failsafe: Ensure this component is attached to a node

	UTrafficNetworkSubsystem* Subsystem = GetWorld()->GetSubsystem<UTrafficNetworkSubsystem>();
	if (!Subsystem) return;

	AIntersectionNode* ChosenDestination = nullptr;
	TArray<ARoadSegment*> FinalPath;

	// 1. DETERMINE DESTINATION
	if (bIsRandomDest)
	{
		TArray<AActor*> AllNodes;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIntersectionNode::StaticClass(), AllNodes);
		
		if (AllNodes.Num() > 1)
		{
			for (int i = 0; i < 10; i++)
			{
				int32 RandomIndex = FMath::RandRange(0, AllNodes.Num() - 1);
				AIntersectionNode* PotentialDest = Cast<AIntersectionNode>(AllNodes[RandomIndex]);
				
				if (PotentialDest && PotentialDest != OwningNode)
				{
					TArray<ARoadSegment*> TestPath = Subsystem->FindPath(OwningNode, PotentialDest);
					if (TestPath.Num() > 0)
					{
						ChosenDestination = PotentialDest;
						FinalPath = TestPath;
						break; 
					}
				}
			}
		}
	}
	else
	{
		ChosenDestination = SpecificDestNode;
		FinalPath = Subsystem->FindPath(OwningNode, ChosenDestination);
	}

	if (!ChosenDestination || FinalPath.Num() == 0) return;

	// 2. NOW WE ACTUALLY SPAWN THE CAR 
	ATrafficManager* Manager = Cast<ATrafficManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATrafficManager::StaticClass()));
	if (Manager)
	{
		Manager->SpawnCar(FinalPath, ChosenDestination); 
	}
}

void UTrafficSpawnerComponent::TriggerRushHour(int32 Amount)
{
	// Add cars to the waiting list instead of spawning them instantly
	QueuedCarsToSpawn += Amount;

	// Start a timer that tries to spawn a car every 0.5 seconds
	if (!GetWorld()->GetTimerManager().IsTimerActive(SpawnQueueTimer))
	{
		GetWorld()->GetTimerManager().SetTimer(SpawnQueueTimer, this, &UTrafficSpawnerComponent::AttemptSpawnFromQueue, 0.5f, true);
	}
}

void UTrafficSpawnerComponent::ProcessRushHourSpawn()
{
	if (RushHourCarsRemaining > 0)
	{
		SpawnVehicleRoutine(); 
		RushHourCarsRemaining--;
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(RushHourTimerHandle);
	}
}
void UTrafficSpawnerComponent::CancelRushHour()
{
	QueuedCarsToSpawn = 0;
	GetWorld()->GetTimerManager().ClearTimer(SpawnQueueTimer);
}