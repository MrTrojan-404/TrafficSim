#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TrafficNetworkSubsystem.generated.h"

class AIntersectionNode;
class ARoadSegment;

// Internal struct for the A* calculation
struct FPathNode
{
	AIntersectionNode* Node;
	float GCost; // Cost from start
	float HCost; // Heuristic cost to end
	float FCost() const { return GCost + HCost; }
	ARoadSegment* SegmentTaken; // The road used to reach this node
	FPathNode* Parent;

	FPathNode(AIntersectionNode* InNode) : Node(InNode), GCost(0), HCost(0), SegmentTaken(nullptr), Parent(nullptr) {}
};

UCLASS()
class TRAFFICSIM_API UTrafficNetworkSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	// The A* Search Algorithm
	UFUNCTION(BlueprintCallable, Category = "Traffic AI")
	TArray<ARoadSegment*> FindPath(AIntersectionNode* StartNode, AIntersectionNode* EndNode);

private:
	void BuildGraph();
    
	UPROPERTY()
	TArray<AIntersectionNode*> GraphNodes;

	UFUNCTION(BlueprintCallable, Category = "Traffic AI | Debug")
	void DebugDrawPath(AIntersectionNode* StartNode, AIntersectionNode* EndNode);
};