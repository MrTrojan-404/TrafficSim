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

	// Start the repeating spawn timer
	// Update: Check if the DestinationNodes array has at least one element!
	if (StartNode && DestinationNodes.Num() > 0 && VehicleClassToSpawn)
	{
		GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &ATrafficSpawner::SpawnVehicle, SpawnInterval, true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TrafficSpawner is missing a StartNode, DestinationNodes, or VehicleClass!"));
	}
}
void ATrafficSpawner::SpawnVehicle()
{
	// Check if the array actually has destinations
	if (!VehicleClassToSpawn || !StartNode || DestinationNodes.Num() == 0) return;

	UTrafficNetworkSubsystem* Subsystem = GetWorld()->GetSubsystem<UTrafficNetworkSubsystem>();
	if (!Subsystem) return;

	// 1. Pick a random destination from the array
	int32 RandomIndex = FMath::RandRange(0, DestinationNodes.Num() - 1);
	AIntersectionNode* TargetDestination = DestinationNodes[RandomIndex];

	// Failsafe in case a null reference snuck into the array or it picked the start node
	if (!TargetDestination || TargetDestination == StartNode) return;

	// 2. Calculate the path using the randomly selected destination
	TArray<ARoadSegment*> Path = Subsystem->FindPath(StartNode, TargetDestination);

	// 3. Only spawn if a valid path exists
	if (Path.Num() > 0)
	{
		// ---> CAPACITY CHECK <---
		ARoadSegment* StartingRoad = Path[0];
		if (StartingRoad && StartingRoad->CurrentVehicleCount >= StartingRoad->MaxCapacity)
		{
			// The road is full! Skip spawning this time and try again on the next timer tick.
			UE_LOG(LogTemp, Warning, TEXT("TrafficSpawner: Spawn aborted. Starting road is full!"));
			return; 
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FVector SpawnLocation = StartNode->GetActorLocation();
		FRotator SpawnRotation = FRotator::ZeroRotator;

		ATrafficVehicle* NewVehicle = GetWorld()->SpawnActor<ATrafficVehicle>(VehicleClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);

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