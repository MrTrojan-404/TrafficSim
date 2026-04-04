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

protected:
	virtual void BeginPlay() override;

	// The vehicle blueprint to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TSubclassOf<ATrafficVehicle> VehicleClassToSpawn;

	// How often to spawn a car (in seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float SpawnInterval = 3.0f;

	// Where the car starts
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Spawning")
	AIntersectionNode* StartNode;

	// Where the car is trying to go
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Spawning")
	AIntersectionNode* DestinationNode;

private:
	FTimerHandle SpawnTimerHandle;

	UFUNCTION()
	void SpawnVehicle();
};