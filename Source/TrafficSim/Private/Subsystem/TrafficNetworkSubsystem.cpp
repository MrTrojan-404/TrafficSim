// Fill out your copyright notice in the Description page of Project Settings.


#include "TrafficSim/Public/Subsystem/TrafficNetworkSubsystem.h"

#include "EngineUtils.h"
#include "TrafficSim/Public/Road/IntersectionNode.h"
#include "TrafficSim/Public/Road/RoadSegment.h"

void UTrafficNetworkSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    
    BuildGraph();
}

void UTrafficNetworkSubsystem::BuildGraph()
{
    GraphNodes.Empty();
    UWorld* World = GetWorld();
    if (!World) return;

    // 1. Gather all nodes and wipe any stale editor data
    for (TActorIterator<AIntersectionNode> It(World); It; ++It)
    {
        It->OutgoingSegments.Empty();
        It->IncomingSegments.Empty();
        GraphNodes.Add(*It);
    }
    
    // 2. Gather all roads and automatically link them to their Start Nodes!
    int32 RoadCount = 0;
    for (TActorIterator<ARoadSegment> It(World); It; ++It)
    {
        if (It->StartNode)
        {
            It->StartNode->OutgoingSegments.AddUnique(*It);
            It->EndNode->IncomingSegments.AddUnique(*It); // Hook up for traffic lights!
        }
        
        // The Two-Way Magic: Reverse the connections for the same physical road
        if (It->bIsTwoWay && It->EndNode)
        {
            It->EndNode->OutgoingSegments.AddUnique(*It);
            It->StartNode->IncomingSegments.AddUnique(*It); 
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TrafficSim: Built graph with %d intersections and linked %d roads."), GraphNodes.Num(), RoadCount);
}

TArray<ARoadSegment*> UTrafficNetworkSubsystem::FindPath(AIntersectionNode* StartNode, AIntersectionNode* EndNode)
{
    TArray<ARoadSegment*> PathSegments;
    if (!StartNode || !EndNode) return PathSegments;

    TArray<FPathNode*> OpenSet;
    TMap<AIntersectionNode*, FPathNode*> AllNodesMap;

    FPathNode* StartRecord = new FPathNode(StartNode);
    StartRecord->GCost = 0.0f;
    StartRecord->HCost = FVector::Distance(StartNode->GetActorLocation(), EndNode->GetActorLocation());
    
    OpenSet.Add(StartRecord);
    AllNodesMap.Add(StartNode, StartRecord);

    FPathNode* CurrentRecord = nullptr;

    while (OpenSet.Num() > 0)
    {
        // Find node with lowest FCost in OpenSet (Using a simple linear search for hackathon scale)
        int32 BestIndex = 0;
        for (int32 i = 1; i < OpenSet.Num(); ++i)
        {
            if (OpenSet[i]->FCost() < OpenSet[BestIndex]->FCost())
            {
                BestIndex = i;
            }
        }

        CurrentRecord = OpenSet[BestIndex];
        OpenSet.RemoveAt(BestIndex);

        // Target Reached! Trace back the path
        if (CurrentRecord->Node == EndNode)
        {
            while (CurrentRecord->Parent != nullptr)
            {
                PathSegments.Insert(CurrentRecord->SegmentTaken, 0); // Insert at beginning to reverse order
                CurrentRecord = CurrentRecord->Parent;
            }
            break; 
        }

        // Check neighbors (Outgoing Segments)
        for (ARoadSegment* Segment : CurrentRecord->Node->OutgoingSegments)
        {
            if (!Segment || !Segment->EndNode) continue;

            AIntersectionNode* NeighborNode = (Segment->StartNode == CurrentRecord->Node) ? Segment->EndNode : Segment->StartNode;
            
            // Calculate movement cost considering dynamic traffic congestion
            float TentativeGCost = CurrentRecord->GCost + Segment->GetRoutingWeight();

            FPathNode* NeighborRecord = nullptr;
            if (AllNodesMap.Contains(NeighborNode))
            {
                NeighborRecord = AllNodesMap[NeighborNode];
                // If we found a worse path to an already evaluated node, skip
                if (TentativeGCost >= NeighborRecord->GCost) continue; 
            }
            else
            {
                NeighborRecord = new FPathNode(NeighborNode);
                AllNodesMap.Add(NeighborNode, NeighborRecord);
                OpenSet.Add(NeighborRecord);
            }

            // Update neighbor with better path data
            NeighborRecord->Parent = CurrentRecord;
            NeighborRecord->SegmentTaken = Segment;
            NeighborRecord->GCost = TentativeGCost;
            NeighborRecord->HCost = FVector::Distance(NeighborNode->GetActorLocation(), EndNode->GetActorLocation());
        }
    }

    // Cleanup memory allocations
    for (auto& Pair : AllNodesMap)
    {
        delete Pair.Value;
    }

    return PathSegments;
}

void UTrafficNetworkSubsystem::DebugDrawPath(AIntersectionNode* StartNode, AIntersectionNode* EndNode)
{
    TArray<ARoadSegment*> Path = FindPath(StartNode, EndNode);

    if (Path.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("TrafficSim Debug: No path found between selected nodes."));
        return;
    }

    // Draw the route
    for (ARoadSegment* Segment : Path)
    {
        if (Segment && Segment->SplineComponent)
        {
            for (int32 i = 0; i < Segment->SplineComponent->GetNumberOfSplinePoints() - 1; i++)
            {
                FVector StartLoc = Segment->SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
                FVector EndLoc = Segment->SplineComponent->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::World);
                
                // Draw a thick green line slightly above the road
                DrawDebugLine(GetWorld(), StartLoc + FVector(0,0,50), EndLoc + FVector(0,0,50), FColor::Green, false, 5.0f, 0, 20.0f);
            }
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("TrafficSim Debug: Path found across %d segments."), Path.Num());
}