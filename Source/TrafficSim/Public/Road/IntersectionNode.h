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

	// Which incoming road currently has a green light?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic AI")
	class ARoadSegment* CurrentGreenRoad;

	// List of roads feeding INTO this intersection
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic AI")
	TArray<class ARoadSegment*> IncomingSegments;
	
	// Called when the player clicks this node
	UFUNCTION(BlueprintCallable, Category = "Traffic AI | Interactivity")
	void PlayerForceLightChange();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ClickableMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UPointLightComponent* TrafficLightVisual;
	
	UFUNCTION()
	void OnIntersectionClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);
protected:
	virtual void BeginPlay() override;

private:
	int32 GreenLightIndex = 0;
	void CycleTrafficLight();
	FTimerHandle LightTimerHandle;
};