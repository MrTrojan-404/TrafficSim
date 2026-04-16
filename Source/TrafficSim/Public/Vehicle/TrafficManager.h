#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "TrafficManager.generated.h"

class ARoadSegment;
class AIntersectionNode;

UCLASS()
class TRAFFICSIM_API ATrafficManager : public AActor
{
	GENERATED_BODY()

public: 
	ATrafficManager();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic")
	TArray<UInstancedStaticMeshComponent*> VehicleISMs; 

	UPROPERTY(EditAnywhere, Category = "Traffic Config")
	TArray<UStaticMesh*> VehicleMeshes;

	UPROPERTY(EditAnywhere, Category = "Traffic Config")
	int32 AmbulanceMeshIndex = -1; // -1 means none assigned yet

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Config | Speeds")
	float CivilianSpeedMin = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Config | Speeds")
	float CivilianSpeedMax = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Config | Speeds")
	float EmergencySpeed = 1800.0f;
	
	// DATA-ORIENTED DESIGN: STRUCT OF ARRAYS
	// The CPU will cache these individual arrays for blistering fast iteration
	TArray<bool> bIsActive;
	TArray<int32> HISMIndex;
	TArray<int32> InstanceIndex;
    
	// Logic Arrays
	TArray<TArray<ARoadSegment*>> CurrentPath;
	TArray<int32> PathIndex;
	TArray<float> CurrentSpeed;
	TArray<float> MaxSpeed;
	TArray<float> DistanceAlongSpline;
	TArray<bool> bTravelingForward;
	TArray<bool> bIsEmergencyVehicle;
	TArray<bool> bIsPullingOver;
    
	// Tracking Arrays
	TArray<float> SpawnTime;
	TArray<float> TimeStuck;
	TArray<float> LastGPSCheckTime;
	TArray<float> CurrentLaneOffset;
    
	// Pointer Arrays
	TArray<AIntersectionNode*> DebugEndNode;
	TArray<ARoadSegment*> CurrentSegment;

	// Total size of the pre-allocated pool
	int32 TotalPoolSize = 0;

	void SpawnCar(TArray<ARoadSegment*> Route, AIntersectionNode* Destination);
	void DespawnCar(int32 CarIndex);

	void RegisterToRoad(int32 CarIndex);
	void UnregisterFromRoad(int32 CarIndex);

	// The raw memory buffer for Batch Updating
	TArray<TArray<FTransform>> TransformBuffers;
	
protected:
	virtual void BeginPlay() override;
};