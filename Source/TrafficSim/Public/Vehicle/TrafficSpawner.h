#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficSpawner.generated.h"

class AIntersectionNode;
class ATrafficVehicle;

UCLASS()
class TRAFFICSIM_API ATrafficSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	ATrafficSpawner();

	// Trigger a massive wave of cars
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void TriggerRushHour(int32 CarCount = 10);
protected:
	virtual void BeginPlay() override;

	// A list of different vehicle blueprints to randomly spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TArray<TSubclassOf<ATrafficVehicle>> VehicleClassesToSpawn;

	// How often to spawn a car (in seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float SpawnInterval = 3.0f;

	// Where the car starts
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Spawning")
	AIntersectionNode* StartNode;

	// A list of possible destinations for the cars to choose from
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Spawning")
	TArray<AIntersectionNode*> DestinationNodes;

private:
	FTimerHandle SpawnTimerHandle;

	UFUNCTION()
	void SpawnVehicle();

	int32 RushHourCarsRemaining = 0;
	FTimerHandle RushHourTimerHandle;

	UFUNCTION()
	void ProcessRushHourSpawn();
};