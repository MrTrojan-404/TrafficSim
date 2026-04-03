#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IntersectionNode.generated.h"

class ARoadSegment;

UCLASS()
class TRAFFICSIM_API AIntersectionNode : public AActor
{
	GENERATED_BODY()
    
public:	
	AIntersectionNode();

	// Visual representation for the editor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* SphereComponent;

	// The road segments that vehicles can take to exit this intersection
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Traffic Graph")
	TArray<ARoadSegment*> OutgoingSegments;
};