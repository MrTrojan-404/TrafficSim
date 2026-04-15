#include "Pawn/RTSCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Controller/TrafficPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Road/RoadSegment.h"

ARTSCameraPawn::ARTSCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    // Build the Component Hierarchy
    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    RootComponent = RootScene;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->bDoCollisionTest = false; // Drones don't clip on buildings!
    SpringArm->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f)); // Look down at a 60-degree angle
    SpringArm->TargetArmLength = 5000.0f;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);

    // Initialize Targets
    TargetZoom = 5000.0f;
    TargetRotation = SpringArm->GetRelativeRotation();
}

void ARTSCameraPawn::BeginPlay()
{
    Super::BeginPlay();

    TargetLocation = GetActorLocation();
    TargetRotation = SpringArm->GetRelativeRotation();

    // Register Enhanced Input
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (CameraMappingContext)
            {
                Subsystem->AddMappingContext(CameraMappingContext, 1);
            }
        }
    }
}

void ARTSCameraPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 1. EDGE SCROLLING LOGIC
    if (bEnableEdgeScroll)
    {
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            if (PC->bShowMouseCursor && 
               !PC->IsInputKeyDown(EKeys::RightMouseButton) && 
               !PC->IsInputKeyDown(EKeys::MiddleMouseButton))
            {
                float MouseX, MouseY;
                if (PC->GetMousePosition(MouseX, MouseY))
                {
                    int32 ViewportX, ViewportY;
                    PC->GetViewportSize(ViewportX, ViewportY);

                    float EdgeBorder = 25.0f; 
                    FVector2D PanInput(0.0f, 0.0f);

                    if (MouseX <= EdgeBorder) PanInput.Y = -1.0f; 
                    else if (MouseX >= ViewportX - EdgeBorder) PanInput.Y = 1.0f; 

                    if (MouseY <= EdgeBorder) PanInput.X = 1.0f; 
                    else if (MouseY >= ViewportY - EdgeBorder) PanInput.X = -1.0f; 

                    if (!PanInput.IsZero())
                    {
                        // Use World Rotation (GetComponentRotation), not Relative
                        FRotator YawRot(0.0f, SpringArm->GetComponentRotation().Yaw, 0.0f);
                        FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
                        FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

                        TargetLocation += (Forward * PanInput.X + Right * PanInput.Y) * EdgeScrollSpeed * DeltaTime;
                    }
                }
            }
        }
    }

    // 2. GOD MODE AUTOPILOT (Perfect Center Orbit)
    if (bIsCinematicMode)
    {
        // ONLY rotate the camera arm around the locked TargetLocation.
        TargetRotation.Yaw += 5.0f * DeltaTime; 
    }
    
    // 3. SMOOTH INTERPOLATION (Applies to EVERYTHING cleanly now)
    SetActorLocation(FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, 10.0f));
    SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetZoom, DeltaTime, 8.0f);
    SpringArm->SetRelativeRotation(FMath::RInterpTo(SpringArm->GetRelativeRotation(), TargetRotation, DeltaTime, 10.0f));
}

void ARTSCameraPawn::ToggleCinematicMode()
{
    bIsCinematicMode = !bIsCinematicMode;

    if (bIsCinematicMode)
    {
        // 1. Dynamically calculate the true center of the city
        FVector CityCenter = FVector::ZeroVector;
        TArray<AActor*> AllIntersections;
        
        // ---> THE FIX: Search for Intersections, not Roads! <---
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIntersectionNode::StaticClass(), AllIntersections);

        if (AllIntersections.Num() > 0)
        {
            FVector TotalLocation = FVector::ZeroVector;
            for (AActor* Node : AllIntersections)
            {
                // Because nodes physically exist at their locations, this returns true coordinates!
                TotalLocation += Node->GetActorLocation();
            }
            
            CityCenter = TotalLocation / AllIntersections.Num();
            CityCenter.Z = 0.0f; 
        }

        // 2. Lock the focal point to the calculated center
        TargetLocation = CityCenter; 
        
        // 3. Pull the zoom way back to fit the whole city in frame
        TargetZoom = 8000.0f; 
        
        // 4. Set a nice, cinematic downward tilt
        TargetRotation.Pitch = -45.0f; 
    }
}
void ARTSCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveAction) EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARTSCameraPawn::OnMove);
        if (RotateAction) EnhancedInputComponent->BindAction(RotateAction, ETriggerEvent::Triggered, this, &ARTSCameraPawn::OnRotate);
        if (ZoomAction) EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ARTSCameraPawn::OnZoom);
    }
}

void ARTSCameraPawn::OnMove(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    // Use World Rotation (GetComponentRotation)
    FRotator YawRot(0.0f, SpringArm->GetComponentRotation().Yaw, 0.0f);
    FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
    FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

    TargetLocation += (Forward * MovementVector.Y + Right * MovementVector.X) * PanSpeed * GetWorld()->GetDeltaSeconds();
}

void ARTSCameraPawn::OnRotate(const FInputActionValue& Value)
{
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        // If the cursor is VISIBLE, we MUST hold Right or Middle click.
        // If the cursor is HIDDEN (Tab locked), this check fails, and we get free rotation!
        if (PC->bShowMouseCursor && !PC->IsInputKeyDown(EKeys::RightMouseButton) && !PC->IsInputKeyDown(EKeys::MiddleMouseButton))
        {
            return; 
        }
    }
    if (ATrafficPlayerController* PC = Cast<ATrafficPlayerController>(GetController()))
    {
        PC->AdvanceTutorial(0);
    }

    // 1. Grab both X and Y mouse movements
    FVector2D RotationVector = Value.Get<FVector2D>();

    // 2. YAW (Orbiting Left/Right using Mouse X)
    TargetRotation.Yaw += RotationVector.X * 100.0f * GetWorld()->GetDeltaSeconds();

    // 3. PITCH (Tilting Up/Down using Mouse Y)
    float NewPitch = TargetRotation.Pitch + (RotationVector.Y * 100.0f * GetWorld()->GetDeltaSeconds());
    TargetRotation.Pitch = FMath::Clamp(NewPitch, MinPitch, MaxPitch);
}

void ARTSCameraPawn::OnZoom(const FInputActionValue& Value)
{
    float ZoomValue = Value.Get<float>();
    TargetZoom = FMath::Clamp(TargetZoom + (ZoomValue * -ZoomSpeed), MinZoom, MaxZoom);
}

void ARTSCameraPawn::SetPanSpeed(float NewSpeed)
{
    PanSpeed = NewSpeed;
}
