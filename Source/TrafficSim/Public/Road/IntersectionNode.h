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
	UStaticMeshComponent* RepresentationMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UPointLightComponent* TrafficLightVisual;
	
	UFUNCTION()
	void OnIntersectionClicked(AActor* TouchedActor, FKey ButtonPressed);
	
	// Toggles the Custom Depth rendering for the highlight material
	UFUNCTION(BlueprintCallable, Category = "Visuals")
	void SetHighlight(int32 StencilValue);

	// ---> SPAWNER SETTINGS <---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic AI | Spawning")
	TSubclassOf<class ATrafficVehicle> VehicleClassToSpawn;

	// How often (in seconds) should this node pump out a car?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic AI | Spawning")
	float SpawnInterval = 2.5f;

	// The functions called by our UI/Controller
	void SetAsRandomSpawner();
	void SetAsSpecificSpawner(AIntersectionNode* DestinationNode);

	// HOVER EVENTS
	UFUNCTION()
	void OnHoverBegin(AActor* TouchedActor);

	UFUNCTION()
	void OnHoverEnd(AActor* TouchedActor);
	
protected:
	virtual void BeginPlay() override;

	// Internal Spawner State
	bool bIsActiveSpawner = false;
	bool bIsRandomDest = true;
	
	UPROPERTY()
	AIntersectionNode* SpecificDestNode = nullptr;

	FTimerHandle SpawnerTimerHandle;

	// The actual function that runs every X seconds
	UFUNCTION()
	void SpawnVehicleRoutine();

	// Memory to ensure hover doesn't overwrite a permanent highlight
	int32 PermanentStencil = 0;
	void ApplyStencil(int32 StencilValue);

	
#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif
	
private:
	int32 GreenLightIndex = 0;
	void CycleTrafficLight();
	FTimerHandle LightTimerHandle;
};