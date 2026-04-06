#include "Component/TrafficSpawnerComponent.h"
#include "Road/IntersectionNode.h"
#include "Vehicle/TrafficVehicle.h"
#include "Subsystem/TrafficNetworkSubsystem.h"
#include "Kismet/GameplayStatics.h"

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

void UTrafficSpawnerComponent::SpawnVehicleRoutine()
{
	if (VehicleClassesToSpawn.Num() == 0 || !bIsActiveSpawner) return;

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

	// 2. NOW WE ACTUALLY SPAWN THE CAR (Using GetOwner() for location)
	FVector SpawnLocation = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 150.0f);
	FRotator SpawnRotation = GetOwner()->GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

	int32 RandomVehicleIndex = FMath::RandRange(0, VehicleClassesToSpawn.Num() - 1);
	TSubclassOf<ATrafficVehicle> SelectedVehicleClass = VehicleClassesToSpawn[RandomVehicleIndex];

	if (!SelectedVehicleClass) return; 

	ATrafficVehicle* NewVehicle = GetWorld()->SpawnActor<ATrafficVehicle>(SelectedVehicleClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (NewVehicle)
	{
		NewVehicle->DebugEndNode = ChosenDestination;
		NewVehicle->SetRoute(FinalPath);
	}
}

void UTrafficSpawnerComponent::TriggerRushHour(int32 CarCount)
{
	if (!bIsActiveSpawner) return;

	RushHourCarsRemaining += CarCount;

	if (!GetWorld()->GetTimerManager().IsTimerActive(RushHourTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(RushHourTimerHandle, this, &UTrafficSpawnerComponent::ProcessRushHourSpawn, 0.2f, true);
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