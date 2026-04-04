// Fill out your copyright notice in the Description page of Project Settings.


#include "TrafficSim/Public/Road/RoadSegment.h"

#include "Components/BoxComponent.h"
#include "Components/SplineMeshComponent.h"
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
	
	// Create the visual barrier
	RoadblockVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoadblockVisual"));
	RoadblockVisual->SetCollisionProfileName(TEXT("NoCollision"));
	RoadblockVisual->SetVisibility(false); // Hidden by default
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

		// ---> WE DELETED THE MANUAL CLEANUP LOOP! Unreal handles it automatically now. <---

		// 2. GENERATION: Build the modular road
		if (RoadMeshAsset)
		{
			int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
			
			for (int32 i = 0; i < NumPoints - 1; ++i)
			{
				USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
				SplineMesh->CreationMethod = EComponentCreationMethod::UserConstructionScript; 
				
				// ---> CRITICAL FIX: Setup Attachment MUST happen before Registering <---
				SplineMesh->SetupAttachment(SplineComponent);

				SplineMesh->SetStaticMesh(RoadMeshAsset);
				SplineMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
				SplineMesh->SetMobility(EComponentMobility::Static);

				// Now that it's attached properly, we register it to the world
				SplineMesh->RegisterComponent();

				// Explicitly zero out any rogue offsets just to be safe
				SplineMesh->SetRelativeTransform(FTransform::Identity); 

				// Widen the road
				SplineMesh->SetStartScale(FVector2D(1.0f, 6.0f));
				SplineMesh->SetEndScale(FVector2D(1.0f, 6.0f));

				// Force the component to respect the default plane's center-pivot bounds
				SplineMesh->SetBoundaryMin(-50.0f);
				SplineMesh->SetBoundaryMax(50.0f);

				FVector StartPos, StartTangent, EndPos, EndTangent;
				SplineComponent->GetLocationAndTangentAtSplinePoint(i, StartPos, StartTangent, ESplineCoordinateSpace::Local);
				SplineComponent->GetLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent, ESplineCoordinateSpace::Local);

				SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
			}
		}
		
		float Length = SplineComponent->GetSplineLength();
		float StopDist = 800.0f; 
		float HeightOffset = 300.0f; 

		// ---> Dynamic curb offset based on Left/Right Hand Traffic <---
		float LightOffset = bDriveOnLeft ? -300.0f : 300.0f;

		// Position Forward Light (Outside curb of the forward lane)
		if (Length > StopDist)
		{
			FVector FwdLoc = SplineComponent->GetLocationAtDistanceAlongSpline(Length - StopDist, ESplineCoordinateSpace::World);
			FVector FwdRight = SplineComponent->GetRightVectorAtDistanceAlongSpline(Length - StopDist, ESplineCoordinateSpace::World);
			ForwardLightMesh->SetWorldLocation(FwdLoc + (FwdRight * LightOffset) + FVector(0, 0, HeightOffset));
		}

		// Position Backward Light (Outside curb of the backward lane)
		if (Length > StopDist)
		{
			FVector BwdLoc = SplineComponent->GetLocationAtDistanceAlongSpline(StopDist, ESplineCoordinateSpace::World);
			FVector BwdRight = SplineComponent->GetRightVectorAtDistanceAlongSpline(StopDist, ESplineCoordinateSpace::World);
			BackwardLightMesh->SetWorldLocation(BwdLoc + (BwdRight * -LightOffset) + FVector(0, 0, HeightOffset));
		}
	}
}

void ARoadSegment::BeginPlay()
{
	Super::BeginPlay();

	// Bind to the entire Actor
	OnClicked.AddDynamic(this, &ARoadSegment::OnRoadClicked);
}

void ARoadSegment::OnRoadClicked(AActor* TouchedActor, FKey ButtonPressed)
{
	ToggleRoadblock();
}

void ARoadSegment::ToggleRoadblock()
{
	bIsBlocked = !bIsBlocked;

	if (bIsBlocked)
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		FHitResult Hit;
		
		// 1. Shoot a laser from the mouse to the road
		if (PC && PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			// 2. Find the exact distance along the spline where the laser hit
			float InputKey = SplineComponent->FindInputKeyClosestToWorldLocation(Hit.ImpactPoint);
			BlockedDistance = SplineComponent->GetDistanceAlongSplineAtSplineInputKey(InputKey);

			// 3. Move the visual barrier to that exact spot
			FVector BlockLocation = SplineComponent->GetLocationAtDistanceAlongSpline(BlockedDistance, ESplineCoordinateSpace::World);
			FRotator BlockRotation = SplineComponent->GetRotationAtDistanceAlongSpline(BlockedDistance, ESplineCoordinateSpace::World);

			if (RoadblockVisual)
			{
				RoadblockVisual->SetWorldLocation(BlockLocation);
				RoadblockVisual->SetWorldRotation(BlockRotation);
				RoadblockVisual->SetVisibility(true);
			}
		}
	}
	else
	{
		if (RoadblockVisual) RoadblockVisual->SetVisibility(false);
	}

	UE_LOG(LogTemp, Warning, TEXT("TrafficSim: Road %s block state changed to %d"), *GetName(), bIsBlocked);
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
	// If the road is blocked, return a massive penalty so A* ignores it
	if (bIsBlocked)
	{
		return 999999.0f; 
	}
	
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

