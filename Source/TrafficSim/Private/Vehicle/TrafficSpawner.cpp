#include "Vehicle/TrafficSpawner.h"

#include "TrafficSim/Public/Road/IntersectionNode.h"
#include "TrafficSim/Public/Subsystem/TrafficNetworkSubsystem.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Road/RoadSegment.h"
#include "Vehicle/TrafficManager.h"

ATrafficSpawner::ATrafficSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATrafficSpawner::BeginPlay()
{
	Super::BeginPlay();

	// Check if BOTH arrays have at least one element!
	if (StartNode && DestinationNodes.Num() > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &ATrafficSpawner::SpawnVehicle, SpawnInterval, true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TrafficSpawner is missing a StartNode, DestinationNodes, or VehicleClasses!"));
	}
}

void ATrafficSpawner::SpawnVehicle()
{
	// 1. Check if both arrays actually have data
	if (!StartNode || DestinationNodes.Num() == 0) return;
	
	UTrafficNetworkSubsystem* Subsystem = GetWorld()->GetSubsystem<UTrafficNetworkSubsystem>();
	if (!Subsystem) return;

	// Pick a random destination
	int32 RandomIndex = FMath::RandRange(0, DestinationNodes.Num() - 1);
	AIntersectionNode* TargetDestination = DestinationNodes[RandomIndex];

	if (!TargetDestination || TargetDestination == StartNode) return;

	TArray<ARoadSegment*> Path = Subsystem->FindPath(StartNode, TargetDestination);


	// 3. Only spawn if a valid path exists
	if (Path.Num() > 0)
	{
		// ---> CAPACITY & ROADBLOCK CHECK <---
		ARoadSegment* StartingRoad = Path[0];
		if (StartingRoad)
		{
			// Check if full
			if (StartingRoad->CurrentVehicleCount >= StartingRoad->MaxCapacity)
			{
				UE_LOG(LogTemp, Warning, TEXT("TrafficSpawner: Spawn aborted. Starting road is full!"));
				return; 
			}
			// Check if blocked by user
			if (StartingRoad->bIsBlocked)
			{
				UE_LOG(LogTemp, Warning, TEXT("TrafficSpawner: Spawn aborted. Starting road has a roadblock!"));
				return;
			}
		}

		// Find the manager and tell it to wake up a pooled car!
		ATrafficManager* Manager = Cast<ATrafficManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATrafficManager::StaticClass()));
		if (Manager)
		{
			Manager->SpawnCar(Path, TargetDestination); // Failsafe, fast, zero memory allocation!
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficSpawner: No path found from %s to %s"), *StartNode->GetName(), *TargetDestination->GetName());
	}
}

void ATrafficSpawner::TriggerRushHour(int32 CarCount)
{
	RushHourCarsRemaining += CarCount;

	// If the rapid timer isn't already running, start it (0.2 seconds per car!)
	if (!GetWorld()->GetTimerManager().IsTimerActive(RushHourTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(RushHourTimerHandle, this, &ATrafficSpawner::ProcessRushHourSpawn, 0.2f, true);
	}
}

void ATrafficSpawner::ProcessRushHourSpawn()
{
	if (RushHourCarsRemaining > 0)
	{
		SpawnVehicle(); // Reuse your exact same robust spawn logic!
		RushHourCarsRemaining--;
	}
	else
	{
		// Wave complete, shut down the rapid timer
		GetWorld()->GetTimerManager().ClearTimer(RushHourTimerHandle);
	}
}