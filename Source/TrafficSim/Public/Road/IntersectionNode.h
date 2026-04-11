#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IntersectionNode.generated.h"

class ARoadSegment;

UENUM(BlueprintType)
enum class ELightOverrideState : uint8
{
	Normal,
	AllRed,
	AllGreen
};

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
	
	// HOVER EVENTS
	UFUNCTION()
	void OnHoverBegin(AActor* TouchedActor);

	UFUNCTION()
	void OnHoverEnd(AActor* TouchedActor);

	// ---> TRAFFIC LIGHT OVERRIDES <---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic AI | Lights")
	ELightOverrideState LightState = ELightOverrideState::Normal;

	UFUNCTION(BlueprintCallable, Category = "Traffic AI | Lights")
	void SetLightState(ELightOverrideState NewState);

	// Helper function to push the colors to the meshes
	void ApplyLightColors();

	// Destroys all connected roads, then destroys itself
	UFUNCTION(BlueprintCallable, Category = "Building")
	void DestroyIntersectionSafe();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UTrafficSpawnerComponent* SpawnerComponent;

	// Unique ID used for Save/Load serialization
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Graph")
	FGuid UniqueID;

	// PASS-THROUGH NODE SETTINGS
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic AI | Lights")
	bool bHasTrafficLights = true;

	UFUNCTION(BlueprintCallable, Category = "Traffic AI | Lights")
	void ToggleTrafficLights();
	
protected:
	virtual void BeginPlay() override;

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