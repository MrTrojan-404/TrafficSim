#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "RoadSegment.generated.h"

class AIntersectionNode;

UCLASS()
class TRAFFICSIM_API ARoadSegment : public AActor
{
	GENERATED_BODY()
    
public:	
	ARoadSegment();
	virtual void OnConstruction(const FTransform& Transform) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USplineComponent* SplineComponent;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Traffic Graph")
	AIntersectionNode* StartNode;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Traffic Graph")
	AIntersectionNode* EndNode;

	// Traffic Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Data")
	int32 MaxCapacity = 10;

	UPROPERTY(BlueprintReadWrite, Category = "Traffic Data")
	int32 CurrentVehicleCount = 0;

	// Calculates the dynamic cost of traversing this road
	UFUNCTION(BlueprintCallable, Category = "Traffic Logic")
	float GetRoutingWeight() const;
};