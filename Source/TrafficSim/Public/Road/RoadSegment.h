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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Data")
	TArray<class ATrafficVehicle*> ActiveVehicles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Data")
	bool bIsTwoWay = true; // Default to two-way now!

	// Split the trackers so cars only care about their own lane
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Data")
	TArray<ATrafficVehicle*> VehiclesForward; // Driving StartNode -> EndNode

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Data")
	TArray<ATrafficVehicle*> VehiclesBackward; // Driving EndNode -> StartNode
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ForwardLightMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BackwardLightMesh;

	// The 3D model used for the road
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
	UStaticMesh* RoadMeshAsset;

	// The intersection will call this to change the color
	void SetIntersectionLightColor(class AIntersectionNode* Intersection, FLinearColor Color);

	// --- ROADBLOCK SYSTEM ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Data")
	bool bIsBlocked = false;

	// The visual barrier (e.g., a cone, barricade, or just a red cube)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* RoadblockVisual;

	UFUNCTION(BlueprintCallable, Category = "Traffic Logic")
	void ToggleRoadblock();

	// Add this variable to remember the roadblock's exact position
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Data")
	float BlockedDistance = 0.0f;

	// Update your click function signature (we are binding to the Actor now)
	UFUNCTION()
	void OnRoadClicked(AActor* TouchedActor, FKey ButtonPressed);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Data")
	bool bDriveOnLeft = true;

	// --- HEATMAP VISUALS ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals | Heatmap")
	FLinearColor EmptyRoadColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f); // Pure Black = No Glow

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals | Heatmap")
	FLinearColor CongestedColor = FLinearColor(5.0f, 0.0f, 0.0f, 1.0f); // High-Intensity Red
	
	// Stores the dynamic materials so we don't recreate them every frame
	UPROPERTY()
	TArray<class UMaterialInstanceDynamic*> HeatmapMaterials;

	// The function to dynamically snap the road between two nodes at runtime
	UFUNCTION(BlueprintCallable, Category = "Building")
	void SetupConnection(class AIntersectionNode* InStartNode, class AIntersectionNode* InEndNode);
protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle HeatmapTimerHandle;

	UFUNCTION()
	void UpdateHeatmap();
	
};