#include "Road/DynamicObstacle.h"
#include "Road/RoadSegment.h"
#include "Components/SplineComponent.h"

ADynamicObstacle::ADynamicObstacle()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
}

void ADynamicObstacle::StartCrossing(ARoadSegment* TargetRoad, float SplineDistance)
{
	if (!IsValid(TargetRoad) || !TargetRoad->SplineComponent)
	{
		Destroy();
		return;
	}

	RoadBeingCrossed = TargetRoad;

	// 1. Tell the road it is currently blocked by an animal/debris!
	RoadBeingCrossed->bHasDynamicObstacle = true;
	RoadBeingCrossed->DynamicObstacleDistance = SplineDistance;

	// 2. Calculate the Left side (Start) and Right side (End) of the road
	FVector CenterLoc = RoadBeingCrossed->SplineComponent->GetLocationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World);
	FVector RightVec = RoadBeingCrossed->SplineComponent->GetRightVectorAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World);
	
	// Assuming the road is roughly 600 units wide (300 each way) + some buffer space for the grass
	StartLoc = CenterLoc - (RightVec * 500.0f);
	EndLoc = CenterLoc + (RightVec * 500.0f);

	SetActorLocation(StartLoc);
	
	// Face the direction it is walking
	FRotator FaceDirection = (EndLoc - StartLoc).Rotation();
	
	// Add a 90-degree offset to fix the imported mesh
	FaceDirection.Yaw += 90.0f;
	
	SetActorRotation(FaceDirection);

	TargetCrossDistance = FVector::Distance(StartLoc, EndLoc);
	CurrentCrossDistance = 0.0f;
}

void ADynamicObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsValid(RoadBeingCrossed))
	{
		Destroy(); // If the player deleted the road while the cow was walking, vaporize the cow!
		return;
	}

	// Move forward
	CurrentCrossDistance += (CrossingSpeed * DeltaTime);
	
	// Interpolate between start and end
	float Alpha = FMath::Clamp(CurrentCrossDistance / TargetCrossDistance, 0.0f, 1.0f);
	FVector NewLocation = FMath::Lerp(StartLoc, EndLoc, Alpha);
	SetActorLocation(NewLocation);

	// Did it reach the other side?
	if (Alpha >= 1.0f)
	{
		// Unblock the road and vanish!
		RoadBeingCrossed->bHasDynamicObstacle = false;
		Destroy();
	}
}