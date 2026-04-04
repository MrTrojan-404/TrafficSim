// Fill out your copyright notice in the Description page of Project Settings.


#include "TrafficSim/Public/Road/RoadSegment.h"
#include "TrafficSim/Public/Road/IntersectionNode.h"


ARoadSegment::ARoadSegment()
{
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	RootComponent = SplineComponent;

	// Create the meshes, turn off collision, and scale them down so they aren't huge
	ForwardLightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForwardLightMesh"));
	ForwardLightMesh->SetupAttachment(RootComponent);
	ForwardLightMesh->SetCollisionProfileName(TEXT("NoCollision"));
	ForwardLightMesh->SetWorldScale3D(FVector(0.5f)); 

	BackwardLightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackwardLightMesh"));
	BackwardLightMesh->SetupAttachment(RootComponent);
	BackwardLightMesh->SetCollisionProfileName(TEXT("NoCollision"));
	BackwardLightMesh->SetWorldScale3D(FVector(0.5f));
}
void ARoadSegment::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SplineComponent)
	{
		// Snap the first spline point to the StartNode
		if (StartNode)
		{
			SplineComponent->SetLocationAtSplinePoint(0, StartNode->GetActorLocation(), ESplineCoordinateSpace::World,
			                                          true);
		}

		// Snap the last spline point to the EndNode
		if (EndNode)
		{
			int32 LastPointIndex = SplineComponent->GetNumberOfSplinePoints() - 1;
			SplineComponent->SetLocationAtSplinePoint(LastPointIndex, EndNode->GetActorLocation(),
			                                          ESplineCoordinateSpace::World, true);
		}

		float Length = SplineComponent->GetSplineLength();
		float StopDist = 800.0f; // Matches your vehicle braking distance!
		float HeightOffset = 300.0f; // How high the light floats above the road

		// Position Forward Light
		if (Length > StopDist)
		{
			FVector FwdLoc = SplineComponent->GetLocationAtDistanceAlongSpline(Length - StopDist, ESplineCoordinateSpace::World);
			FVector FwdRight = SplineComponent->GetRightVectorAtDistanceAlongSpline(Length - StopDist, ESplineCoordinateSpace::World);
			ForwardLightMesh->SetWorldLocation(FwdLoc + (FwdRight * 200.0f) + FVector(0, 0, HeightOffset));
		}

		// Position Backward Light
		if (Length > StopDist)
		{
			FVector BwdLoc = SplineComponent->GetLocationAtDistanceAlongSpline(StopDist, ESplineCoordinateSpace::World);
			FVector BwdRight = SplineComponent->GetRightVectorAtDistanceAlongSpline(StopDist, ESplineCoordinateSpace::World);
			BackwardLightMesh->SetWorldLocation(BwdLoc + (BwdRight * -200.0f) + FVector(0, 0, HeightOffset));
		}
	}
}

void ARoadSegment::SetIntersectionLightColor(AIntersectionNode* Intersection, FLinearColor Color)
{
	UStaticMeshComponent* TargetMesh = nullptr;

	if (Intersection == EndNode) TargetMesh = ForwardLightMesh;
	else if (Intersection == StartNode) TargetMesh = BackwardLightMesh;

	if (TargetMesh)
	{
		// This looks for a parameter named "LightColor" in your material and changes it!
		// We convert FLinearColor to FVector because materials prefer vector math.
		TargetMesh->SetVectorParameterValueOnMaterials(TEXT("LightColor"), FVector(Color.R, Color.G, Color.B));
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
