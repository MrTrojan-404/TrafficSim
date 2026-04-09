#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DynamicObstacle.generated.h"

class ARoadSegment;

UCLASS()
class TRAFFICSIM_API ADynamicObstacle : public AActor
{
	GENERATED_BODY()
	
public:	
	ADynamicObstacle();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Components")
	USkeletalMeshComponent* MeshComponent;

	// The speed the obstacle crosses the road
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	float CrossingSpeed = 100.0f;

	// Called by the spawner to initialize the walk
	void StartCrossing(ARoadSegment* TargetRoad, float SplineDistance);

protected:
	UPROPERTY()
	ARoadSegment* RoadBeingCrossed;

	float CurrentCrossDistance = 0.0f;
	float TargetCrossDistance = 0.0f;
	FVector StartLoc;
	FVector EndLoc;
};