#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "RTSCameraPawn.generated.h"

UCLASS()
class TRAFFICSIM_API ARTSCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    ARTSCameraPawn();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category = "Camera Settings")
    void SetPanSpeed(float NewSpeed);
    
protected:
    virtual void BeginPlay() override;

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USceneComponent* RootScene;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USpringArmComponent* SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UCameraComponent* Camera;

    // Enhanced Input
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputMappingContext* CameraMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* MoveAction;   // Axis2D (WASD)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* RotateAction; // Axis1D (Q/E or Middle Mouse)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* ZoomAction;   // Axis1D (Scroll Wheel)

    // Drone Settings
    UPROPERTY(EditAnywhere, Category = "Camera Settings")
    float PanSpeed = 4000.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Settings")
    float ZoomSpeed = 1500.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Settings")
    float EdgeScrollSpeed = 3000.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Settings")
    bool bEnableEdgeScroll = true;

    UPROPERTY(EditAnywhere, Category = "Camera Settings")
    float MinZoom = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Settings")
    float MaxZoom = 50000.0f;

    // CAMERA TILT LIMITS
    UPROPERTY(EditAnywhere, Category = "Camera Settings")
    float MinPitch = -85.0f; // Looking almost straight down

    UPROPERTY(EditAnywhere, Category = "Camera Settings")
    float MaxPitch = -10.0f; // Looking almost straight forward
    
private:
    // Input Functions
    void OnMove(const FInputActionValue& Value);
    void OnRotate(const FInputActionValue& Value);
    void OnZoom(const FInputActionValue& Value);

    // Interpolation Targets for butter-smooth movement
    FVector TargetLocation;
    float TargetZoom;
    FRotator TargetRotation;
};