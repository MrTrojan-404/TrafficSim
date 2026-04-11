#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TrafficSaveGame.generated.h"

// Struct to hold Intersection Data
USTRUCT(BlueprintType)
struct FNodeSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid NodeID; // Unique identifier for the graph

	UPROPERTY()
	FTransform NodeTransform;
};

// Struct to hold Road Data
USTRUCT(BlueprintType)
struct FRoadSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid StartNodeID;

	UPROPERTY()
	FGuid EndNodeID;
};

UCLASS()
class TRAFFICSIM_API UTrafficSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TArray<FNodeSaveData> SavedNodes;

	UPROPERTY()
	TArray<FRoadSaveData> SavedRoads;

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	bool bHasCompletedTutorial = false;

	// Hardware & Environment Settings
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	float SavedPanSpeed = 4000.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	float SavedTimeOfDay = 0.0f;
};