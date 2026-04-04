#include "Vehicle/TrafficSpawner.h"

#include "TrafficSim/Public/Vehicle/TrafficVehicle.h"
#include "TrafficSim/Public/Road/IntersectionNode.h"
#include "TrafficSim/Public/Subsystem/TrafficNetworkSubsystem.h"
#include "TimerManager.h"

ATrafficSpawner::ATrafficSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATrafficSpawner::BeginPlay()
{
	Super::BeginPlay();

	// Start the repeating spawn timer
	if (StartNode && DestinationNode && VehicleClassToSpawn)
	{
		GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &ATrafficSpawner::SpawnVehicle, SpawnInterval, true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TrafficSpawner is missing a StartNode, DestinationNode, or VehicleClass!"));
	}
}

void ATrafficSpawner::SpawnVehicle()
{
	if (!VehicleClassToSpawn || !StartNode || !DestinationNode) return;

	UTrafficNetworkSubsystem* Subsystem = GetWorld()->GetSubsystem<UTrafficNetworkSubsystem>();
	if (!Subsystem) return;

	// 1. Calculate the path
	TArray<ARoadSegment*> Path = Subsystem->FindPath(StartNode, DestinationNode);

	// 2. Only spawn if a valid path exists
	if (Path.Num() > 0)
	{
		// Spawn parameters (always spawn, even if colliding initially)
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// Spawn the vehicle at the start node's location
		FVector SpawnLocation = StartNode->GetActorLocation();
		FRotator SpawnRotation = FRotator::ZeroRotator;

		ATrafficVehicle* NewVehicle = GetWorld()->SpawnActor<ATrafficVehicle>(VehicleClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);

		if (NewVehicle)
		{
			// 3. Give the vehicle its route! 
			// Your existing RegisterToRoad() logic will automatically snap it to the correct lane.
			NewVehicle->SetRoute(Path);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficSpawner: No path found from %s to %s"), *StartNode->GetName(), *DestinationNode->GetName());
	}
}