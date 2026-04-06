// Fill out your copyright notice in the Description page of Project Settings.


#include "TrafficSim/Public/Road/RoadSegment.h"

#include "Components/BoxComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Controller/TrafficPlayerController.h"
#include "TrafficSim/Public/Road/IntersectionNode.h"
#include "Vehicle/TrafficVehicle.h"


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

	if (SplineComponent) SplineComponent->SetMobility(EComponentMobility::Movable);
	if (ForwardLightMesh) ForwardLightMesh->SetMobility(EComponentMobility::Movable);
	if (BackwardLightMesh) BackwardLightMesh->SetMobility(EComponentMobility::Movable);
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
		float LightOffset = bDriveOnLeft ? -150.0f : 150.0f;

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

	// ---> NEW HEATMAP SETUP <---
	// 1. Find all the spline meshes we generated in the Construction Script
	TArray<USplineMeshComponent*> SplineMeshes;
	GetComponents<USplineMeshComponent>(SplineMeshes);

	// 2. Convert their materials to Dynamic Instances and save them
	for (USplineMeshComponent* Mesh : SplineMeshes)
	{
		if (Mesh)
		{
			UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(0);
			if (MID)
			{
				HeatmapMaterials.Add(MID);
			}
		}
	}

	// 3. Start the heatmap update loop (Running twice a second is highly optimized)
	GetWorld()->GetTimerManager().SetTimer(HeatmapTimerHandle, this, &ARoadSegment::UpdateHeatmap, 0.5f, true);
}

void ARoadSegment::UpdateHeatmap()
{
	float CongestionRatio = 0.0f;

	if (bIsBlocked)
	{
		CongestionRatio = 1.0f;
	}
	else if (MaxCapacity > 0)
	{
		CongestionRatio = FMath::Clamp((float)CurrentVehicleCount / (float)MaxCapacity, 0.0f, 1.0f);
	}

	// ---> Apply an exponential curve! <---
	// 0.1 cubed is 0.001 (invisible). 0.5 cubed is 0.125 (subtle). 0.9 cubed is 0.72 (bright!).
	float VisualIntensityCurve = FMath::Pow(CongestionRatio, 3.0f);

	// Use our new curved ratio for the color blend
	FLinearColor CurrentColor = FMath::Lerp(EmptyRoadColor, CongestedColor, VisualIntensityCurve);

	for (UMaterialInstanceDynamic* MID : HeatmapMaterials)
	{
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("RoadColor"), CurrentColor);
		}
	}
}

void ARoadSegment::OnRoadClicked(AActor* TouchedActor, FKey ButtonPressed)
{
	ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetWorld()->GetFirstPlayerController());
    
	// Only allow roadblocks in Simulate Mode!
	if (PC && PC->CurrentGameMode == ETrafficGameMode::Simulate)
	{
		ToggleRoadblock();
	}
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

void ARoadSegment::SetupConnection(AIntersectionNode* InStartNode, AIntersectionNode* InEndNode)
{
	if (!InStartNode || !InEndNode || !SplineComponent) return;

	StartNode = InStartNode;
	EndNode = InEndNode;

	// Use absolute world coordinates! No relative math needed.
	TArray<FVector> NewSplinePoints = { StartNode->GetActorLocation(), EndNode->GetActorLocation() };

	SplineComponent->SetSplinePoints(NewSplinePoints, ESplineCoordinateSpace::World, true);

	SplineComponent->SetSplinePointType(0, ESplinePointType::Linear);
	SplineComponent->SetSplinePointType(1, ESplinePointType::Linear);
	SplineComponent->UpdateSpline();

	if (StartNode)
	{
		StartNode->OutgoingSegments.AddUnique(this);
		StartNode->IncomingSegments.AddUnique(this);
	}
    
	if (EndNode)
	{
		EndNode->OutgoingSegments.AddUnique(this);
		EndNode->IncomingSegments.AddUnique(this);
	}
}

void ARoadSegment::DestroyRoadSafe()
{
	//Copy the arrays, then empty them BEFORE destroying the cars
	TArray<ATrafficVehicle*> ForwardCopy = VehiclesForward;
	TArray<ATrafficVehicle*> BackwardCopy = VehiclesBackward;

	VehiclesForward.Empty();
	VehiclesBackward.Empty();

	// Vaporize the cars
	for (ATrafficVehicle* V : ForwardCopy) if (IsValid(V)) V->Destroy();
	for (ATrafficVehicle* V : BackwardCopy) if (IsValid(V)) V->Destroy();

	// Unhook from Nodes
	if (StartNode)
	{
		StartNode->OutgoingSegments.Remove(this);
		StartNode->IncomingSegments.Remove(this);
	}

	if (EndNode)
	{
		EndNode->OutgoingSegments.Remove(this);
		EndNode->IncomingSegments.Remove(this);
	}

	Destroy();
}

void ARoadSegment::UpdateDriveSide(bool bDriveLeft)
{
	bDriveOnLeft = bDriveLeft;

	// Re-calculate the curb offsets for the traffic lights
	float Length = SplineComponent->GetSplineLength();
	float StopDist = 800.0f; 
	float HeightOffset = 300.0f; 
	float LightOffset = bDriveOnLeft ? -150.0f : 150.0f;

	if (Length > StopDist)
	{
		// Move Forward Light
		FVector FwdLoc = SplineComponent->GetLocationAtDistanceAlongSpline(Length - StopDist, ESplineCoordinateSpace::World);
		FVector FwdRight = SplineComponent->GetRightVectorAtDistanceAlongSpline(Length - StopDist, ESplineCoordinateSpace::World);
		ForwardLightMesh->SetWorldLocation(FwdLoc + (FwdRight * LightOffset) + FVector(0, 0, HeightOffset));

		// Move Backward Light
		FVector BwdLoc = SplineComponent->GetLocationAtDistanceAlongSpline(StopDist, ESplineCoordinateSpace::World);
		FVector BwdRight = SplineComponent->GetRightVectorAtDistanceAlongSpline(StopDist, ESplineCoordinateSpace::World);
		BackwardLightMesh->SetWorldLocation(BwdLoc + (BwdRight * -LightOffset) + FVector(0, 0, HeightOffset));
	}
}