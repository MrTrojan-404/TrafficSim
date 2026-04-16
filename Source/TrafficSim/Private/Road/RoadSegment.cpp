// Fill out your copyright notice in the Description page of Project Settings.


#include "TrafficSim/Public/Road/RoadSegment.h"

#include "Components/BoxComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Controller/TrafficPlayerController.h"
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

	if (SplineComponent) SplineComponent->SetMobility(EComponentMobility::Movable);
	if (ForwardLightMesh) ForwardLightMesh->SetMobility(EComponentMobility::Movable);
	if (BackwardLightMesh) BackwardLightMesh->SetMobility(EComponentMobility::Movable);

	// Initialize the Lights
	// FORWARD LIGHT SETUP 
	ForwardStreetLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("ForwardStreetLight"));
	ForwardStreetLight->SetupAttachment(ForwardLightMesh);
	ForwardStreetLight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); 
	ForwardStreetLight->SetIntensity(0.0f); 
	ForwardStreetLight->SetLightColor(FLinearColor(1.0f, 0.85f, 0.6f));
	ForwardStreetLight->SetInnerConeAngle(60.0f); // Softens the center
	ForwardStreetLight->SetOuterConeAngle(85.0f); // 85 degrees is massive
	ForwardStreetLight->SetAttenuationRadius(3500.0f); // Reaches 3.5x further down the road
	ForwardStreetLight->SetCastShadows(false);

	// BACKWARD LIGHT SETUP
	BackwardStreetLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("BackwardStreetLight"));
	BackwardStreetLight->SetupAttachment(BackwardLightMesh);
	BackwardStreetLight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); 
	BackwardStreetLight->SetIntensity(0.0f); 
	BackwardStreetLight->SetLightColor(FLinearColor(1.0f, 0.85f, 0.6f));
	BackwardStreetLight->SetInnerConeAngle(60.0f);
	BackwardStreetLight->SetOuterConeAngle(85.0f);
	BackwardStreetLight->SetAttenuationRadius(3500.0f);
	BackwardStreetLight->SetCastShadows(false);
}
void ARoadSegment::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SplineComponent)
	{
		// Snap the first spline point to the StartNode
		if (StartNode) SplineComponent->SetLocationAtSplinePoint(0, StartNode->GetActorLocation(), ESplineCoordinateSpace::World, true);

		// Snap the last spline point to the EndNode
		if (EndNode)
		{
			int32 LastPointIndex = SplineComponent->GetNumberOfSplinePoints() - 1;
			SplineComponent->SetLocationAtSplinePoint(LastPointIndex, EndNode->GetActorLocation(), ESplineCoordinateSpace::World, true);
		}
	}
}

void ARoadSegment::BuildRoadGeometry()
{
    if (!SplineComponent) return;

    // 1. GENERATION: Build the modular road
    if (RoadMeshAsset)
    {
        int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
        
        for (int32 i = 0; i < NumPoints - 1; ++i)
        {
            // Notice we dropped the "UserConstructionScript" flag!
            USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
            SplineMesh->SetupAttachment(SplineComponent);
            SplineMesh->SetStaticMesh(RoadMeshAsset);
            SplineMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
            SplineMesh->SetMobility(EComponentMobility::Movable);
            SplineMesh->RegisterComponent();

            SplineMesh->SetRelativeTransform(FTransform::Identity); 
            SplineMesh->SetStartScale(FVector2D(1.0f, 6.0f));
            SplineMesh->SetEndScale(FVector2D(1.0f, 6.0f));
            SplineMesh->SetBoundaryMin(-50.0f);
            SplineMesh->SetBoundaryMax(50.0f);

            FVector StartPos, StartTangent, EndPos, EndTangent;
            SplineComponent->GetLocationAndTangentAtSplinePoint(i, StartPos, StartTangent, ESplineCoordinateSpace::Local);
            SplineComponent->GetLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent, ESplineCoordinateSpace::Local);

            SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
        }
    }
    
    // 2. Position the Lights
    float Length = SplineComponent->GetSplineLength();
    float StopDist = 800.0f; 
    float HeightOffset = 300.0f; 
    float LightOffset = bDriveOnLeft ? -150.0f : 150.0f;

    if (Length > StopDist)
    {
        FVector FwdLoc = SplineComponent->GetLocationAtDistanceAlongSpline(Length - StopDist, ESplineCoordinateSpace::World);
        FVector FwdRight = SplineComponent->GetRightVectorAtDistanceAlongSpline(Length - StopDist, ESplineCoordinateSpace::World);
        ForwardLightMesh->SetWorldLocation(FwdLoc + (FwdRight * LightOffset) + FVector(0, 0, HeightOffset));

        FVector BwdLoc = SplineComponent->GetLocationAtDistanceAlongSpline(StopDist, ESplineCoordinateSpace::World);
        FVector BwdRight = SplineComponent->GetRightVectorAtDistanceAlongSpline(StopDist, ESplineCoordinateSpace::World);
        BackwardLightMesh->SetWorldLocation(BwdLoc + (BwdRight * -LightOffset) + FVector(0, 0, HeightOffset));
    }
}
void ARoadSegment::BeginPlay()
{
	Super::BeginPlay();

	BuildRoadGeometry();
	
	// Bind to the entire Actor
	OnClicked.AddDynamic(this, &ARoadSegment::OnRoadClicked);

	OnBeginCursorOver.AddDynamic(this, &ARoadSegment::OnHoverBegin);
	OnEndCursorOver.AddDynamic(this, &ARoadSegment::OnHoverEnd);
	
	// HEATMAP SETUP 
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
	GetWorld()->GetTimerManager().SetTimer(HeatmapTimerHandle, this, &ARoadSegment::UpdateHeatmap, 0.1f, true);
}

void ARoadSegment::SetHighlight(int32 StencilValue)
{
	TArray<USplineMeshComponent*> SplineMeshes;
	GetComponents<USplineMeshComponent>(SplineMeshes);

	for (USplineMeshComponent* Mesh : SplineMeshes)
	{
		if (Mesh)
		{
			Mesh->SetRenderCustomDepth(StencilValue > 0);
			Mesh->SetCustomDepthStencilValue(StencilValue);
		}
	}
}

void ARoadSegment::OnHoverBegin(AActor* TouchedActor)
{
	ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetWorld()->GetFirstPlayerController());
    
	if (PC && PC->CurrentGameMode == ETrafficGameMode::Delete)
	{
		SetHighlight(250); // Red for Delete Mode
	}
	else if (!bIsBlocked)
	{
		SetHighlight(253); // Standard White/Blue hover for other modes
	}
}

void ARoadSegment::OnHoverEnd(AActor* TouchedActor)
{
	// If the road has a roadblock on it, restore the pink highlight!
	if (bIsBlocked)
	{
		SetHighlight(250); 
	}
	else
	{
		// Otherwise, turn the highlight off completely
		SetHighlight(0);
	}
}

void ARoadSegment::UpdateHeatmap()
{
	ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetWorld()->GetFirstPlayerController());

	// --- NEW: DATA-DRIVEN SMART STREETLIGHTS ---
	if (PC && PC->CurrentTimeOfDay >= 0.6f) // If it is Nighttime
	{
		// If math says cars are here, turn on. Otherwise, turn off.
		float TargetIntensity = (CurrentVehicleCount > 0) ? 15000.0f : 0.0f;
        
		if (ForwardStreetLight && ForwardStreetLight->Intensity != TargetIntensity) 
			ForwardStreetLight->SetIntensity(TargetIntensity);
            
		if (BackwardStreetLight && BackwardStreetLight->Intensity != TargetIntensity) 
			BackwardStreetLight->SetIntensity(TargetIntensity);
	}
	else
	{
		// Daytime: Force lights off
		if (ForwardStreetLight && ForwardStreetLight->Intensity > 0.0f) ForwardStreetLight->SetIntensity(0.0f);
		if (BackwardStreetLight && BackwardStreetLight->Intensity > 0.0f) BackwardStreetLight->SetIntensity(0.0f);
	}
	
	//  Check both the Master Switch and the Local Switch 
	if ((PC && !PC->bMasterHeatmapEnabled) || bDisableHeatmap)
	{
		for (UMaterialInstanceDynamic* MID : HeatmapMaterials)
		{
			if (MID) MID->SetVectorParameterValue(TEXT("RoadColor"), EmptyRoadColor);
		}
		return;
	}
	
	float CongestionRatio = 0.0f;

	if (bIsBlocked) CongestionRatio = 1.0f;
	else if (MaxCapacity > 0) CongestionRatio = FMath::Clamp((float)CurrentVehicleCount / (float)MaxCapacity, 0.0f, 1.0f);

	float VisualIntensityCurve = FMath::Pow(CongestionRatio, 3.0f);

	// ACCESSIBILITY CHECK
	if (PC && PC->bEnableHeatmapPulse && VisualIntensityCurve > 0.1f)
	{
		// Maps sine wave to fluctuate smoothly between 0.4 and 1.0
		float Throb = (FMath::Sin(GetWorld()->GetTimeSeconds() * 5.0f) * 0.5f) + 0.7f;
		VisualIntensityCurve *= Throb;
	}

	FLinearColor CurrentColor = FMath::Lerp(EmptyRoadColor, CongestedColor, VisualIntensityCurve);

	for (UMaterialInstanceDynamic* MID : HeatmapMaterials)
	{
		if (MID) MID->SetVectorParameterValue(TEXT("RoadColor"), CurrentColor);
	}
}

void ARoadSegment::OnRoadClicked(AActor* TouchedActor, FKey ButtonPressed)
{
	ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetWorld()->GetFirstPlayerController());
    
	// Only allow roadblocks in Simulate Mode!
	if (PC && PC->CurrentGameMode == ETrafficGameMode::Simulate)
	{
		ToggleRoadblock();
		PC->AdvanceTutorial(9);
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
	// Just clear the integer arrays. The TrafficManager will automatically see the 
	// road is missing and despawn the cars!
	VehiclesForwardIndices.Empty();
	VehiclesBackwardIndices.Empty();

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