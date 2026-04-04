#include "Vehicle/TrafficSpawner.h"

#include "TrafficSim/Public/Vehicle/TrafficVehicle.h"
#include "TrafficSim/Public/Road/IntersectionNode.h"
#include "TrafficSim/Public/Subsystem/TrafficNetworkSubsystem.h"
#include "TimerManager.h"
#include "Road/RoadSegment.h"

ATrafficSpawner::ATrafficSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATrafficSpawner::BeginPlay()
{
	Super::BeginPlay();

	// Check if BOTH arrays have at least one element!
	if (StartNode && DestinationNodes.Num() > 0 && VehicleClassesToSpawn.Num() > 0)
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
	if (VehicleClassesToSpawn.Num() == 0 || !StartNode || DestinationNodes.Num() == 0) return;

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

		// --->  Pick a random vehicle class from our new array <---
		int32 RandomVehicleIndex = FMath::RandRange(0, VehicleClassesToSpawn.Num() - 1);
		TSubclassOf<ATrafficVehicle> SelectedVehicleClass = VehicleClassesToSpawn[RandomVehicleIndex];

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FVector SpawnLocation = StartNode->GetActorLocation();
		FRotator SpawnRotation = FRotator::ZeroRotator;

		// ---> Pass in the SelectedVehicleClass <---
		ATrafficVehicle* NewVehicle = GetWorld()->SpawnActor<ATrafficVehicle>(SelectedVehicleClass, SpawnLocation, SpawnRotation, SpawnParams);

		if (NewVehicle)
		{
			NewVehicle->DebugEndNode = TargetDestination;
			NewVehicle->SetRoute(Path);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficSpawner: No path found from %s to %s"), *StartNode->GetName(), *TargetDestination->GetName());
	}
}