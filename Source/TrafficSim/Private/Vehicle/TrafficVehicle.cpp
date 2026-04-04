// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/TrafficVehicle.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Road/IntersectionNode.h"
#include "Road/RoadSegment.h"
#include "Subsystem/TrafficNetworkSubsystem.h"

ATrafficVehicle::ATrafficVehicle()
{
    PrimaryActorTick.bCanEverTick = true;

    VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    RootComponent = VehicleMesh;
    
    // Disable physics to save massive CPU overhead
    VehicleMesh->SetSimulatePhysics(false);
    VehicleMesh->SetCollisionProfileName(TEXT("NoCollision")); 

    DistanceAlongSpline = 0.0f;
    PathIndex = 0;
}

void ATrafficVehicle::BeginPlay()
{
    Super::BeginPlay();
    CurrentSpeed = 0.0f;
    TargetSpeed = MaxSpeed;

    // Quick Hackathon Debug: Get a route immediately if nodes are assigned
    if (DebugStartNode && DebugEndNode)
    {
        UTrafficNetworkSubsystem* Subsystem = GetWorld()->GetSubsystem<UTrafficNetworkSubsystem>();
        if (Subsystem)
        {
            TArray<ARoadSegment*> Path = Subsystem->FindPath(DebugStartNode, DebugEndNode);
            SetRoute(Path);
        }
    }
}

void ATrafficVehicle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    MoveAlongSpline(DeltaTime);
}

void ATrafficVehicle::RegisterToRoad()
{
    if (!CurrentSegment || !CurrentSegment->StartNode || !CurrentSegment->EndNode) return;

    AIntersectionNode* TargetNode = nullptr;

    // If there is another road after this one, our target is the intersection connecting them
    if (PathIndex + 1 < CurrentPath.Num())
    {
        ARoadSegment* NextSegment = CurrentPath[PathIndex + 1];
        
        // Find the shared node between our current road and the next road
        if (CurrentSegment->EndNode == NextSegment->StartNode || CurrentSegment->EndNode == NextSegment->EndNode)
        {
            TargetNode = CurrentSegment->EndNode;
        }
        else
        {
            TargetNode = CurrentSegment->StartNode;
        }
    }
    else
    {
        // If this is the last road in the path, our target is our final destination!
        TargetNode = DebugEndNode; 
    }

    // Now we know our direction with 100% mathematical certainty
    bTravelingForward = (TargetNode == CurrentSegment->EndNode);

    // Setup starting distance and add to the correct lane array
    if (bTravelingForward)
    {
        DistanceAlongSpline = 0.0f;
        CurrentSegment->VehiclesForward.Add(this);
    }
    else
    {
        DistanceAlongSpline = CurrentSegment->SplineComponent->GetSplineLength();
        CurrentSegment->VehiclesBackward.Add(this);
    }

    CurrentSegment->CurrentVehicleCount++;
}

void ATrafficVehicle::UnregisterFromRoad()
{
    if (!CurrentSegment) return;

    if (bTravelingForward)
    {
        CurrentSegment->VehiclesForward.Remove(this);
    }
    else
    {
        CurrentSegment->VehiclesBackward.Remove(this);
    }
    
    CurrentSegment->CurrentVehicleCount--;
}

void ATrafficVehicle::SetRoute(TArray<ARoadSegment*> NewRoute)
{
    CurrentPath = NewRoute;
    PathIndex = 0;

    if (CurrentPath.Num() > 0)
    {
        CurrentSegment = CurrentPath[0];
        RegisterToRoad(); // Use the new function!
    }
}

void ATrafficVehicle::MoveAlongSpline(float DeltaTime)
{
    if (!CurrentSegment || !CurrentSegment->SplineComponent) return;

    TargetSpeed = MaxSpeed;
    float DistanceToCarAhead = 999999.0f;
    float SegmentLength = CurrentSegment->SplineComponent->GetSplineLength();

    // 1. WHICH LANE ARE WE IN? Get the correct array of cars
    TArray<ATrafficVehicle*>& RelevantVehicles = bTravelingForward ? CurrentSegment->VehiclesForward : CurrentSegment->VehiclesBackward;

  // 2. CHECK FOR CARS AHEAD
    for (ATrafficVehicle* OtherCar : RelevantVehicles)
    {
        if (OtherCar == this) continue;

        float Gap = 0.0f;
        if (bTravelingForward && OtherCar->DistanceAlongSpline > this->DistanceAlongSpline)
        {
            Gap = OtherCar->DistanceAlongSpline - this->DistanceAlongSpline;
        }
        else if (!bTravelingForward && OtherCar->DistanceAlongSpline < this->DistanceAlongSpline)
        {
            Gap = this->DistanceAlongSpline - OtherCar->DistanceAlongSpline;
        }

        if (Gap > 0.0f && Gap < DistanceToCarAhead)
        {
            DistanceToCarAhead = Gap;
        }
    }

    if (DistanceToCarAhead < MinFollowingDistance) TargetSpeed = 0.0f; 

    // 3. FIXED ROADBLOCK LOGIC (Only stop if the block is strictly IN FRONT of us)
    if (CurrentSegment->bIsBlocked)
    {
        float BlockDist = CurrentSegment->BlockedDistance;
        bool bIsBlockAhead = false;
        float DistToBlock = 0.0f;

        if (bTravelingForward && DistanceAlongSpline < BlockDist)
        {
            bIsBlockAhead = true;
            DistToBlock = BlockDist - DistanceAlongSpline;
        }
        else if (!bTravelingForward && DistanceAlongSpline > BlockDist)
        {
            bIsBlockAhead = true;
            DistToBlock = DistanceAlongSpline - BlockDist;
        }

        // Only stop if the barrier is ahead AND we are close to it
        if (bIsBlockAhead && DistToBlock < (MinFollowingDistance + 150.0f)) 
        {
            TargetSpeed = 0.0f;
        }
    }

    // 4. TRAFFIC LIGHT & DYNAMIC REROUTING
    float DistanceToIntersection = bTravelingForward ? (SegmentLength - DistanceAlongSpline) : DistanceAlongSpline;
    float IntersectionStopDistance = 800.0f; 
    AIntersectionNode* ApproachingNode = bTravelingForward ? CurrentSegment->EndNode : CurrentSegment->StartNode;

    if (DistanceToIntersection < IntersectionStopDistance)
    {
        if (ApproachingNode && ApproachingNode->CurrentGreenRoad != CurrentSegment)
        {
            TargetSpeed = 0.0f; // Red light!

            // ---> NEW DYNAMIC REROUTING <---
            // While waiting at the light, peek at the NEXT road in our path array
            if (PathIndex + 1 < CurrentPath.Num())
            {
                ARoadSegment* NextSegment = CurrentPath[PathIndex + 1];
                
                // If the user just dropped a roadblock on our next turn...
                if (NextSegment && NextSegment->bIsBlocked)
                {
                    UTrafficNetworkSubsystem* Subsystem = GetWorld()->GetSubsystem<UTrafficNetworkSubsystem>();
                    if (Subsystem && DebugEndNode)
                    {
                        // Ask A* for a new route starting from the intersection we are currently sitting at
                        TArray<ARoadSegment*> DetourPath = Subsystem->FindPath(ApproachingNode, DebugEndNode);
                        
                        if (DetourPath.Num() > 0)
                        {
                            // Strip out all old upcoming roads and append the new detour!
                            CurrentPath.RemoveAt(PathIndex + 1, CurrentPath.Num() - (PathIndex + 1));
                            CurrentPath.Append(DetourPath);
                            
                            UE_LOG(LogTemp, Warning, TEXT("TrafficSim: Vehicle rerouted to avoid roadblock!"));
                        }
                    }
                }
            }
        }
    }

    // Smoothly adjust speed, but brake HARD if TargetSpeed is 0
    float InterpSpeed = (TargetSpeed == 0.0f) ? 10.0f : 3.0f;
    CurrentSpeed = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaTime, InterpSpeed);

    // 4. MOVEMENT (which might be labeled // 5 in your current file)
    if (bTravelingForward) DistanceAlongSpline += (CurrentSpeed * DeltaTime);
    else DistanceAlongSpline -= (CurrentSpeed * DeltaTime);

    // 5. ROAD TRANSITIONS
    bool bReachedEnd = bTravelingForward ? (DistanceAlongSpline >= SegmentLength) : (DistanceAlongSpline <= 0.0f);
    if (bReachedEnd)
    {
        UnregisterFromRoad();

        PathIndex++;
        if (PathIndex < CurrentPath.Num())
        {
            CurrentSegment = CurrentPath[PathIndex];
            RegisterToRoad(); 
        }
        else
        {
            Destroy(); // Destination Reached
            return;
        }
    }

    // 6. UPDATE 3D TRANSFORM (LANE OFFSET LOGIC)
    if (CurrentSegment && CurrentSpeed > 0.1f) 
    {
        FVector BaseLocation = CurrentSegment->SplineComponent->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
        FRotator BaseRotation = CurrentSegment->SplineComponent->GetRotationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
        FVector RightVector = CurrentSegment->SplineComponent->GetRightVectorAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
        
        // Push the car 200 units to the right depending on its direction
        float LaneOffset = bTravelingForward ? 150.0f : -150.0f; 
        FVector OffsetLocation = BaseLocation + (RightVector * LaneOffset);

        // If driving backward mathematically, we need to flip the car 180 degrees visually
        FRotator FinalRotation = bTravelingForward ? BaseRotation : (BaseRotation + FRotator(0, 180, 0));

        SetActorLocationAndRotation(OffsetLocation, FinalRotation);
    }
}