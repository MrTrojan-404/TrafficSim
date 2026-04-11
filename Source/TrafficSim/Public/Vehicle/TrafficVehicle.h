#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TrafficVehicle.generated.h"

class ARoadSegment;

UCLASS()
class TRAFFICSIM_API ATrafficVehicle : public APawn
{
	GENERATED_BODY()

public:
	ATrafficVehicle();

	virtual void Tick(float DeltaTime) override;

	// The visual mesh of the car
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* VehicleMesh;

	// The route the car is currently following
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic AI")
	TArray<ARoadSegment*> CurrentPath;

	// Movement Dynamics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic AI")
	float MaxSpeed = 1000.0f; // Unreal Units per second

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic AI")
	float CurrentSpeed = 0.0f;

	// Called by the AI Controller's Behavior Tree to assign a new route
	UFUNCTION(BlueprintCallable, Category = "Traffic AI")
	void SetRoute(TArray<ARoadSegment*> NewRoute);

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Traffic AI | Debug")
	class AIntersectionNode* DebugStartNode;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Traffic AI | Debug")
	AIntersectionNode* DebugEndNode;

	float TargetSpeed;
	float MinFollowingDistance = 500.0f;

	bool bTravelingForward;
	void RegisterToRoad();
	void UnregisterFromRoad();

	// Tracks the last time this car ran A* so we don't spam the CPU
	float LastGPSCheckTime = 0.0f;

	// Is this an Ambulance/Police car?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic AI | Emergency")
	bool bIsEmergencyVehicle = false;

	// State tracker for yielding
	bool bIsPullingOver = false;

	// Tracks when the car spawned to calculate trip duration
	float SpawnTime = 0.0f;

	float DistanceAlongSpline;

protected:
	virtual void BeginPlay() override;

private:
	// Internal tracking for spline movement
	ARoadSegment* CurrentSegment;
	int32 PathIndex;

	void MoveAlongSpline(float DeltaTime);

	float CurrentLaneOffset = 0.0f;
};