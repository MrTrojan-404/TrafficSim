// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/TrafficVehicle.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Controller/TrafficPlayerController.h"
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
    
    SpawnTime = GetWorld()->GetTimeSeconds();
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
    if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        PC->RegisterSpawnedCar();
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
    if (!IsValid(CurrentSegment) || !IsValid(CurrentSegment->StartNode) || !IsValid(CurrentSegment->EndNode)) return;
    
    AIntersectionNode* TargetNode = nullptr;

    // If there is another road after this one, our target is the intersection connecting them
    if (PathIndex + 1 < CurrentPath.Num())
    {
        ARoadSegment* NextSegment = CurrentPath[PathIndex + 1];
        
        //  Make sure the next road wasn't deleted by the user
        if (IsValid(NextSegment))
        {
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
            TargetNode = DebugEndNode; // Fallback to final destination
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

    // 1: Did the user delete the road we are currently on, or our destination?
    if (!IsValid(CurrentSegment) || !IsValid(CurrentSegment->SplineComponent) || !IsValid(DebugEndNode))
    {
        Destroy();
        return;
    }
    
    TargetSpeed = MaxSpeed;
    float DistanceToCarAhead = 999999.0f;
    float SegmentLength = CurrentSegment->SplineComponent->GetSplineLength();

    // 1. WHICH LANE ARE WE IN? Get the correct array of cars
    TArray<ATrafficVehicle*>& RelevantVehicles = bTravelingForward ? CurrentSegment->VehiclesForward : CurrentSegment->VehiclesBackward;

    // 2. CHECK FOR CARS (Ahead and Behind!)
    bIsPullingOver = false;
    for (ATrafficVehicle* OtherCar : RelevantVehicles)
    {
        if (OtherCar == this) continue;

        // Ignore cars that have been vaporized
        if (!IsValid(OtherCar) || OtherCar == this) continue;
        
        //  ONLY Ambulances get to ignore cars that have pulled over
        if (bIsEmergencyVehicle && OtherCar->bIsPullingOver) continue;

        // Positive Gap = OtherCar is Ahead. Negative Gap = OtherCar is Behind.
        float Gap = bTravelingForward ? (OtherCar->DistanceAlongSpline - this->DistanceAlongSpline) : (this->DistanceAlongSpline - OtherCar->DistanceAlongSpline);

        // Check front bumper for normal braking (Normal cars WILL respect pulled-over cars!)
        if (Gap > 0.0f && Gap < DistanceToCarAhead)
        {
            DistanceToCarAhead = Gap;
        }

        //  The "Emergency Corridor" Hold 
        if (!bIsEmergencyVehicle && OtherCar->bIsEmergencyVehicle)
        {
            // If the siren is up to 2500 units behind us, OR up to 800 units ahead of us
            // (meaning it passed us but got trapped at the same red light), stay on the shoulder!
            if (Gap > -2500.0f && Gap < 800.0f)
            {
                bIsPullingOver = true;
            }
        }
    }

    if (DistanceToCarAhead < MinFollowingDistance) TargetSpeed = 0.0f; 
    
    // If a siren is behind us, slam on the brakes so the offset math can slide us to the curb
    if (bIsPullingOver) TargetSpeed = 0.0f;

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
    //  THE COW CHECK 
    if (CurrentSegment->bHasDynamicObstacle)
    {
        float CowDist = CurrentSegment->DynamicObstacleDistance;
        bool bIsCowAhead = false;
        float DistToCow = 0.0f;

        if (bTravelingForward && DistanceAlongSpline < CowDist)
        {
            bIsCowAhead = true;
            DistToCow = CowDist - DistanceAlongSpline;
        }
        else if (!bTravelingForward && DistanceAlongSpline > CowDist)
        {
            bIsCowAhead = true;
            DistToCow = DistanceAlongSpline - CowDist;
        }

        // Stop for the cow! (Give it a bit more buffer space than a normal car)
        if (bIsCowAhead && DistToCow < (MinFollowingDistance + 300.0f)) 
        {
            TargetSpeed = 0.0f;
        }
    }

    // 4. DYNAMIC GPS REROUTING (Works for the whole lane!) 
    if (PathIndex + 1 < CurrentPath.Num())
    {
        ARoadSegment* NextSegment = CurrentPath[PathIndex + 1];

        // ---> THE FIX: Check if the road is blocked OR at maximum capacity! <---
        if (IsValid(NextSegment) && (NextSegment->bIsBlocked || NextSegment->CurrentVehicleCount >= NextSegment->MaxCapacity))
        {
            float CurrentTime = GetWorld()->GetTimeSeconds();
            if (CurrentTime - LastGPSCheckTime > 2.0f)
            {
                LastGPSCheckTime = CurrentTime;

                AIntersectionNode* UpcomingIntersection = bTravelingForward ? CurrentSegment->EndNode : CurrentSegment->StartNode;
                UTrafficNetworkSubsystem* Subsystem = GetWorld()->GetSubsystem<UTrafficNetworkSubsystem>();
                
                if (Subsystem && DebugEndNode && UpcomingIntersection)
                {
                    TArray<ARoadSegment*> DetourPath = Subsystem->FindPath(UpcomingIntersection, DebugEndNode);
                    
                    if (DetourPath.Num() > 0 && DetourPath[0] != NextSegment)
                    {
                        CurrentPath.RemoveAt(PathIndex + 1, CurrentPath.Num() - (PathIndex + 1));
                        CurrentPath.Append(DetourPath);
                    }
                }
            }
        }
    }

    // 5. TRAFFIC LIGHT & GRIDLOCK CHECK
    float DistanceToIntersection = bTravelingForward ? (SegmentLength - DistanceAlongSpline) : DistanceAlongSpline;
    float IntersectionStopDistance = 800.0f; 
    AIntersectionNode* ApproachingNode = bTravelingForward ? CurrentSegment->EndNode : CurrentSegment->StartNode;

    if (DistanceToIntersection < IntersectionStopDistance)
    {
        bool bRedLightStop = false;
        bool bGridlockStop = false;

        // Rule A: The Red Light Check
        // Only check the light if the intersection actually has them
        if (ApproachingNode && ApproachingNode->bHasTrafficLights) 
        {
            if (ApproachingNode->LightState == ELightOverrideState::AllRed)
            {
                // Master STOP
                bRedLightStop = true;
            }
            else if (ApproachingNode->LightState == ELightOverrideState::AllGreen)
            {
                // Master GO
                bRedLightStop = false;
            }
            else if (ApproachingNode->CurrentGreenRoad != CurrentSegment)
            {
                // Normal Operation
                bRedLightStop = true; 
            }
        }

        // Rule B: "Don't Block the Box" (Gridlock Check)
        if (PathIndex + 1 < CurrentPath.Num())
        {
            ARoadSegment* NextSegment = CurrentPath[PathIndex + 1];
            
            // ---> SAFETY FIX 3: Make sure the next road exists before checking its capacity! <---
            if (IsValid(NextSegment)) 
            {
                if (NextSegment->bIsBlocked || NextSegment->CurrentVehicleCount >= NextSegment->MaxCapacity)
                {
                    bGridlockStop = true;
                }
            }
        }

        // Split the Override
        // Sirens ignore red lights, but NO ONE ignores physics (Gridlock/Full Roads)
        if (bIsEmergencyVehicle)
        {
            bRedLightStop = false; 
        }

        // If either condition is true, hit the brakes
        if (bRedLightStop || bGridlockStop)
        {
            TargetSpeed = 0.0f;
        }
    }

    // Smoothly adjust speed, but brake HARD if TargetSpeed is 0
    float InterpSpeed = (TargetSpeed == 0.0f) ? 10.0f : 3.0f;
    CurrentSpeed = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaTime, InterpSpeed);

    // 6. MOVEMENT (which might be labeled // 5 in your current file)
    if (bTravelingForward) DistanceAlongSpline += (CurrentSpeed * DeltaTime);
    else DistanceAlongSpline -= (CurrentSpeed * DeltaTime);

    // 7. ROAD TRANSITIONS
    bool bReachedEnd = bTravelingForward ? (DistanceAlongSpline >= SegmentLength) : (DistanceAlongSpline <= 0.0f);
    if (bReachedEnd)
    {
        UnregisterFromRoad();

        PathIndex++;
        if (PathIndex < CurrentPath.Num())
        {
            CurrentSegment = CurrentPath[PathIndex];
            
            // ---> SAFETY FIX 2: Did the user delete the road we are about to turn onto? <---
            if (!IsValid(CurrentSegment) || !IsValid(CurrentSegment->StartNode) || !IsValid(CurrentSegment->EndNode))
            {
                Destroy(); // Vaporize the car before it accesses dead memory!
                return;
            }

            RegisterToRoad(); 
        }
        else
        {
            // ---> NEW: Report the completed trip before dying! <---
            if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetWorld()->GetFirstPlayerController()))
            {
                float TripDuration = GetWorld()->GetTimeSeconds() - SpawnTime;
                PC->RegisterCompletedTrip(TripDuration);
            }

            Destroy(); // Destination Reached
            return;
        }
    }
    // 8. UPDATE 3D TRANSFORM (LANE OFFSET LOGIC)
    if (CurrentSegment) 
    {
        FVector BaseLocation = CurrentSegment->SplineComponent->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
        FRotator BaseRotation = CurrentSegment->SplineComponent->GetRotationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
        FVector RightVector = CurrentSegment->SplineComponent->GetRightVectorAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
        
        // Get the "Up" direction of the road
        FVector UpVector = CurrentSegment->SplineComponent->GetUpVectorAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);

        // Dynamic Lane Math based on the Road's configuration
        float BaseOffset = CurrentSegment->bDriveOnLeft ? -150.0f : 150.0f;
        float TargetLaneOffset = bTravelingForward ? BaseOffset : -BaseOffset; 

        if (bIsPullingOver)
        {
            TargetLaneOffset *= 1.8f; 
        }

        CurrentLaneOffset = FMath::FInterpTo(CurrentLaneOffset, TargetLaneOffset, DeltaTime, 2.0f);
        
        // Add the vertical offset here
        float TireHeightOffset = 30.0f; 
        
        // Calculate the final 3D position (Base + Left/Right + Up/Down)
        FVector OffsetLocation = BaseLocation + (RightVector * CurrentLaneOffset) + (UpVector * TireHeightOffset);

        FRotator FinalRotation = bTravelingForward ? BaseRotation : (BaseRotation + FRotator(0, 180, 0));

        SetActorLocationAndRotation(OffsetLocation, FinalRotation);
    }
    
    // 9. THE GRIDLOCK RELIEF VALVE
    if (CurrentSpeed < 5.0f)
    {
        TimeStuck += DeltaTime;
        
        // If trapped in unmoving gridlock for 20 seconds, give up and clear the road
        if (TimeStuck > 20.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("TrafficSim: Vehicle despawned to relieve cascading gridlock!"));
            UnregisterFromRoad();
            Destroy();
            return;
        }
    }
    else
    {
        // Reset the timer the moment traffic starts flowing again
        TimeStuck = 0.0f;
    }
} 