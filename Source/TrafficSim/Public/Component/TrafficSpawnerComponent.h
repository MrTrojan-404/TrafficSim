#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TrafficSpawnerComponent.generated.h"

class AIntersectionNode;
class ATrafficVehicle;

UCLASS( ClassGroup=(Traffic), meta=(BlueprintSpawnableComponent) )
class TRAFFICSIM_API UTrafficSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTrafficSpawnerComponent();

	// ---> CONFIGURATION <---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TArray<TSubclassOf<ATrafficVehicle>> VehicleClassesToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float SpawnInterval = 2.5f;

	// ---> CORE FUNCTIONS <---
	void SetAsRandomSpawner();
	void SetAsSpecificSpawner(AIntersectionNode* DestinationNode);
	void TriggerRushHour(int32 CarCount = 10);

	// Is this component currently pumping out cars?
	bool bIsActiveSpawner = false;

	int32 QueuedCarsToSpawn = 0;
	FTimerHandle SpawnQueueTimer;

	UFUNCTION()
	void AttemptSpawnFromQueue();

	// Cancel the queue
	UFUNCTION(BlueprintCallable, Category = "Traffic AI | Spawner")
	void CancelRushHour();
	
protected:
	bool bIsRandomDest = true;
	
	UPROPERTY()
	AIntersectionNode* SpecificDestNode = nullptr;

	FTimerHandle SpawnerTimerHandle;

	UFUNCTION()
	void SpawnVehicleRoutine();

	// ---> RUSH HOUR STATE <---
	int32 RushHourCarsRemaining = 0;
	FTimerHandle RushHourTimerHandle;

	UFUNCTION()
	void ProcessRushHourSpawn();
};