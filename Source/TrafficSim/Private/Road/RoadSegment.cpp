// Fill out your copyright notice in the Description page of Project Settings.


#include "TrafficSim/Public/Road/RoadSegment.h"

#include "TrafficSim/Public/Road/IntersectionNode.h"


ARoadSegment::ARoadSegment()
{
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	RootComponent = SplineComponent;
}

void ARoadSegment::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SplineComponent)
	{
		// Snap the first spline point to the StartNode
		if (StartNode)
		{
			SplineComponent->SetLocationAtSplinePoint(0, StartNode->GetActorLocation(), ESplineCoordinateSpace::World, true);
		}
        
		// Snap the last spline point to the EndNode
		if (EndNode)
		{
			int32 LastPointIndex = SplineComponent->GetNumberOfSplinePoints() - 1;
			SplineComponent->SetLocationAtSplinePoint(LastPointIndex, EndNode->GetActorLocation(), ESplineCoordinateSpace::World, true);
		}
	}
}

float ARoadSegment::GetRoutingWeight() const
{
	// Base weight is the physical length of the road
	float BaseWeight = SplineComponent->GetSplineLength();
    
	// Calculate congestion (0.0 to 1.0)
	float CongestionRatio = FMath::Clamp((float)CurrentVehicleCount / (float)FMath::Max(1, MaxCapacity), 0.0f, 1.0f);
    
	// Exponential penalty for heavy traffic to force detours
	float TrafficPenalty = FMath::Lerp(1.0f, 5.0f, FMath::Pow(CongestionRatio, 2.0f));

	// If the road is completely blocked by a user widget, return an astronomically high weight
	if (CongestionRatio >= 1.0f) 
	{
		return 999999.0f;
	}

	return BaseWeight * TrafficPenalty;
}