#include "Vehicle/TrafficManager.h"
#include "Road/RoadSegment.h"
#include "Road/IntersectionNode.h"
#include "Controller/TrafficPlayerController.h"
#include "Subsystem/TrafficNetworkSubsystem.h"

ATrafficManager::ATrafficManager()
{
    PrimaryActorTick.bCanEverTick = true;
    // Tick at the end of the frame to ensure roads and lights are updated first
    PrimaryActorTick.TickGroup = TG_PostUpdateWork; 
}

void ATrafficManager::BeginPlay()
{
    Super::BeginPlay();

    int32 MaxPoolSizePerMesh = 2000; 
    TotalPoolSize = MaxPoolSizePerMesh * VehicleMeshes.Num();

    // 1. PRE-ALLOCATE CONTIGUOUS MEMORY BLOCKS
    bIsActive.Init(false, TotalPoolSize);
    HISMIndex.Init(0, TotalPoolSize);
    InstanceIndex.Init(0, TotalPoolSize);
    CurrentPath.Init(TArray<ARoadSegment*>(), TotalPoolSize);
    PathIndex.Init(0, TotalPoolSize);
    CurrentSpeed.Init(0.0f, TotalPoolSize);
    DistanceAlongSpline.Init(0.0f, TotalPoolSize);
    bTravelingForward.Init(true, TotalPoolSize);
    bIsEmergencyVehicle.Init(false, TotalPoolSize);
    bIsPullingOver.Init(false, TotalPoolSize);
    SpawnTime.Init(0.0f, TotalPoolSize);
    TimeStuck.Init(0.0f, TotalPoolSize);
    LastGPSCheckTime.Init(0.0f, TotalPoolSize);
    CurrentLaneOffset.Init(0.0f, TotalPoolSize);
    DebugEndNode.Init(nullptr, TotalPoolSize);
    CurrentSegment.Init(nullptr, TotalPoolSize);

    // 2. Generate the ISMs and map the indices
    FTransform HiddenTransform(FRotator::ZeroRotator, FVector(0, 0, -10000.0f), FVector::ZeroVector);
    int32 GlobalIndex = 0;

    // Resize the main buffer array to hold one list per vehicle type
    TransformBuffers.SetNum(VehicleMeshes.Num());

    for (int32 i = 0; i < VehicleMeshes.Num(); i++)
    {
        UInstancedStaticMeshComponent* NewISM = NewObject<UInstancedStaticMeshComponent>(this);
        NewISM->RegisterComponent();
        NewISM->SetStaticMesh(VehicleMeshes[i]);
        NewISM->SetCollisionEnabled(ECollisionEnabled::NoCollision); 
        NewISM->SetCastShadow(false);
        NewISM->SetCullDistances(1000, 20000);
        
        VehicleISMs.Add(NewISM);

        // Pre-fill the C++ memory buffer with hidden transforms
        TransformBuffers[i].Init(HiddenTransform, MaxPoolSizePerMesh);

        for (int j = 0; j < MaxPoolSizePerMesh; j++)
        {
            int32 GPUIndex = NewISM->AddInstance(HiddenTransform);
            
            HISMIndex[GlobalIndex] = i;
            InstanceIndex[GlobalIndex] = GPUIndex;
            
            GlobalIndex++;
        }
    }
}

void ATrafficManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Reset the pathfinding budget every frame
    static int32 PathfindsThisFrame = 0;
    PathfindsThisFrame = 0;
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    UTrafficNetworkSubsystem* Subsystem = GetWorld()->GetSubsystem<UTrafficNetworkSubsystem>();

    for (int32 i = 0; i < TotalPoolSize; i++)
    {
        // PURE CACHE EFFICIENCY: The CPU checks a tiny boolean array. 
        if (!bIsActive[i] || !CurrentSegment[i] || !CurrentSegment[i]->SplineComponent) continue;

        if (!IsValid(CurrentSegment[i]) || !IsValid(DebugEndNode[i]))
        {
            DespawnCar(i);
            continue;
        }

        float TargetSpeed = 1000.0f; 
        float MinFollowingDistance = 500.0f;
        float DistanceToCarAhead = 999999.0f;
        float SegmentLength = CurrentSegment[i]->SplineComponent->GetSplineLength();

        TArray<int32>& RelevantLane = bTravelingForward[i] ? CurrentSegment[i]->VehiclesForwardIndices : CurrentSegment[i]->VehiclesBackwardIndices;

        bIsPullingOver[i] = false;

        // --- 1. GAP CHECKING ---
        for (int32 OtherCarIndex : RelevantLane)
        {
            if (OtherCarIndex == i || !bIsActive[OtherCarIndex]) continue; 

            float Gap = bTravelingForward[i] ? (DistanceAlongSpline[OtherCarIndex] - DistanceAlongSpline[i]) : (DistanceAlongSpline[i] - DistanceAlongSpline[OtherCarIndex]);

            if (Gap > 0.0f && Gap < DistanceToCarAhead) DistanceToCarAhead = Gap;
        }

        if (DistanceToCarAhead < MinFollowingDistance) TargetSpeed = 0.0f;

        // --- 2. ROADBLOCK CHECK ---
        if (CurrentSegment[i]->bIsBlocked)
        {
            float DistToBlock = bTravelingForward[i] ? (CurrentSegment[i]->BlockedDistance - DistanceAlongSpline[i]) : (DistanceAlongSpline[i] - CurrentSegment[i]->BlockedDistance);
            if (DistToBlock > 0.0f && DistToBlock < MinFollowingDistance + 150.0f) TargetSpeed = 0.0f;
        }

        // --- 3. GPS REROUTING ---
        if (PathIndex[i] + 1 < CurrentPath[i].Num())
        {
            ARoadSegment* NextSegment = CurrentPath[i][PathIndex[i] + 1];
            if (IsValid(NextSegment) && (NextSegment->bIsBlocked || NextSegment->CurrentVehicleCount >= NextSegment->MaxCapacity))
            {
                if (CurrentTime - LastGPSCheckTime[i] > 2.0f)
                {
                    LastGPSCheckTime[i] = CurrentTime;
                    AIntersectionNode* UpcomingNode = bTravelingForward[i] ? CurrentSegment[i]->EndNode : CurrentSegment[i]->StartNode;
                    
                    // Only allow 1 car to do this heavy math per frame!
                    if (Subsystem && UpcomingNode && DebugEndNode[i] && PathfindsThisFrame < 1)
                    {
                        TArray<ARoadSegment*> Detour = Subsystem->FindPath(UpcomingNode, DebugEndNode[i]);
                        PathfindsThisFrame++; // Spend the budget!

                        if (Detour.Num() > 0 && Detour[0] != NextSegment)
                        {
                            CurrentPath[i].RemoveAt(PathIndex[i] + 1, CurrentPath[i].Num() - (PathIndex[i] + 1));
                            CurrentPath[i].Append(Detour);
                        }
                    }
                }
            }
        }

        // --- 4. TRAFFIC LIGHTS ---
        float DistanceToIntersection = bTravelingForward[i] ? (SegmentLength - DistanceAlongSpline[i]) : DistanceAlongSpline[i];
        if (DistanceToIntersection < 800.0f)
        {
            AIntersectionNode* ApproachingNode = bTravelingForward[i] ? CurrentSegment[i]->EndNode : CurrentSegment[i]->StartNode;
            bool bRedLightStop = false;

            if (ApproachingNode && ApproachingNode->bHasTrafficLights)
            {
                if (ApproachingNode->LightState == ELightOverrideState::AllRed) bRedLightStop = true;
                else if (ApproachingNode->LightState != ELightOverrideState::AllGreen && ApproachingNode->CurrentGreenRoad != CurrentSegment[i]) bRedLightStop = true;
            }

            // Gridlock Check
            if (PathIndex[i] + 1 < CurrentPath[i].Num())
            {
                ARoadSegment* NextSegment = CurrentPath[i][PathIndex[i] + 1];
                if (IsValid(NextSegment) && (NextSegment->bIsBlocked || NextSegment->CurrentVehicleCount >= NextSegment->MaxCapacity)) bRedLightStop = true;
            }

            if (bRedLightStop && !bIsEmergencyVehicle[i]) TargetSpeed = 0.0f;
        }

        // --- 5. MOVEMENT & INTERPOLATION ---
        float InterpSpeed = (TargetSpeed == 0.0f) ? 10.0f : 3.0f;
        CurrentSpeed[i] = FMath::FInterpTo(CurrentSpeed[i], TargetSpeed, DeltaTime, InterpSpeed);

        if (bTravelingForward[i]) DistanceAlongSpline[i] += (CurrentSpeed[i] * DeltaTime);
        else DistanceAlongSpline[i] -= (CurrentSpeed[i] * DeltaTime);


        // --- 6. ROAD TRANSITIONS ---
        bool bReachedEnd = bTravelingForward[i] ? (DistanceAlongSpline[i] >= SegmentLength) : (DistanceAlongSpline[i] <= 0.0f);
        if (bReachedEnd)
        {
            UnregisterFromRoad(i);
            PathIndex[i]++;

            if (PathIndex[i] < CurrentPath[i].Num())
            {
                CurrentSegment[i] = CurrentPath[i][PathIndex[i]];
                RegisterToRoad(i);
            }
            else
            {
                if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetWorld()->GetFirstPlayerController()))
                {
                    PC->RegisterCompletedTrip(CurrentTime - SpawnTime[i]);
                }
                DespawnCar(i);
                continue; 
            }
        }

        // --- 7. BATCH BUFFER UPDATE ---
        
        FTransform SplineTransform = CurrentSegment[i]->SplineComponent->GetTransformAtDistanceAlongSpline(DistanceAlongSpline[i], ESplineCoordinateSpace::World);
        
        FVector BaseLoc = SplineTransform.GetLocation();
        FRotator BaseRot = SplineTransform.Rotator();
        FVector RightVec = SplineTransform.GetRotation().GetRightVector();
        FVector UpVec = SplineTransform.GetRotation().GetUpVector();

        float BaseOffset = CurrentSegment[i]->bDriveOnLeft ? -150.0f : 150.0f;
        float TargetLaneOffset = bTravelingForward[i] ? BaseOffset : -BaseOffset;

        CurrentLaneOffset[i] = FMath::FInterpTo(CurrentLaneOffset[i], TargetLaneOffset, DeltaTime, 2.0f);
        FVector FinalLoc = BaseLoc + (RightVec * CurrentLaneOffset[i]) + (UpVec * 30.0f);
        FRotator FinalRot = bTravelingForward[i] ? BaseRot : (BaseRot + FRotator(0, 180, 0));

        // OPTIMIZATION: Write directly to our raw C++ array instead of calling the Unreal Component!
        TransformBuffers[HISMIndex[i]][InstanceIndex[i]] = FTransform(FinalRot, FinalLoc, FVector::OneVector);

        // --- 8. GRIDLOCK RELIEF ---
        if (CurrentSpeed[i] < 5.0f)
        {
            TimeStuck[i] += DeltaTime;
            if (TimeStuck[i] > 20.0f) DespawnCar(i);
        }
        else TimeStuck[i] = 0.0f;
        
    } // <--- THIS IS THE END OF YOUR MASSIVE FOR-LOOP

    // ---> THE BATCH EXECUTION <---
    // Hand the raw memory array directly to the GPU in one massive block
    for (int32 v = 0; v < VehicleISMs.Num(); v++)
    {
        if (VehicleISMs[v])
        {
            // Parameters: StartIndex, TransformArray, bWorldSpace, bMarkRenderStateDirty, bTeleport
            VehicleISMs[v]->BatchUpdateInstancesTransforms(0, TransformBuffers[v], true, true, true);
        }
    }
}

void ATrafficManager::SpawnCar(TArray<ARoadSegment*> Route, AIntersectionNode* Destination)
{
    if (Route.Num() == 0 || !Destination) return;

    // 1. Pick a random vehicle type!
    int32 DesiredMeshIndex = FMath::RandRange(0, VehicleMeshes.Num() - 1);

    // 2. Find an inactive car that matches this exact mesh type
    for (int i = 0; i < TotalPoolSize; i++)
    {
        if (!bIsActive[i] && HISMIndex[i] == DesiredMeshIndex)
        {
            bIsActive[i] = true;
            CurrentPath[i] = Route;
            PathIndex[i] = 0;
            CurrentSegment[i] = Route[0];
            DebugEndNode[i] = Destination;
            CurrentSpeed[i] = 0.0f;
            SpawnTime[i] = GetWorld()->GetTimeSeconds();
            TimeStuck[i] = 0.0f;
            bIsEmergencyVehicle[i] = (FMath::FRand() > 0.95f);
            
            RegisterToRoad(i);
            return;
        }
    }
}

void ATrafficManager::DespawnCar(int32 CarIndex)
{
    bIsActive[CarIndex] = false;
    UnregisterFromRoad(CarIndex);
    
    // Write the underground location to the buffer array
    FTransform Hidden(FRotator::ZeroRotator, FVector(0, 0, -10000.0f), FVector::ZeroVector);
    TransformBuffers[HISMIndex[CarIndex]][InstanceIndex[CarIndex]] = Hidden;
    
    // We don't need to call UpdateTransform here! 
    // It will automatically get pushed to the GPU at the end of the frame by the BatchUpdate!
}

void ATrafficManager::RegisterToRoad(int32 CarIndex)
{
    if (!CurrentSegment[CarIndex]) return;

    AIntersectionNode* TargetNode = nullptr;
    if (PathIndex[CarIndex] + 1 < CurrentPath[CarIndex].Num())
    {
        ARoadSegment* NextSeg = CurrentPath[CarIndex][PathIndex[CarIndex] + 1];
        if (IsValid(NextSeg)) 
        {
            TargetNode = (CurrentSegment[CarIndex]->EndNode == NextSeg->StartNode || CurrentSegment[CarIndex]->EndNode == NextSeg->EndNode) ? CurrentSegment[CarIndex]->EndNode : CurrentSegment[CarIndex]->StartNode;
        }
        else TargetNode = DebugEndNode[CarIndex];
    }
    else TargetNode = DebugEndNode[CarIndex];

    bTravelingForward[CarIndex] = (TargetNode == CurrentSegment[CarIndex]->EndNode);

    if (bTravelingForward[CarIndex])
    {
        DistanceAlongSpline[CarIndex] = 0.0f;
        CurrentSegment[CarIndex]->VehiclesForwardIndices.Add(CarIndex);
    }
    else
    {
        DistanceAlongSpline[CarIndex] = CurrentSegment[CarIndex]->SplineComponent->GetSplineLength();
        CurrentSegment[CarIndex]->VehiclesBackwardIndices.Add(CarIndex);
    }
    CurrentSegment[CarIndex]->CurrentVehicleCount++;
}

void ATrafficManager::UnregisterFromRoad(int32 CarIndex)
{
    if (!CurrentSegment[CarIndex]) return;

    if (bTravelingForward[CarIndex]) CurrentSegment[CarIndex]->VehiclesForwardIndices.Remove(CarIndex);
    else CurrentSegment[CarIndex]->VehiclesBackwardIndices.Remove(CarIndex);

    CurrentSegment[CarIndex]->CurrentVehicleCount--;
}