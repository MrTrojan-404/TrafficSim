#include "Pawn/RTSCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Controller/TrafficPlayerController.h"

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
            //The cursor MUST be visible to allow edge scrolling
            if (PC->bShowMouseCursor && 
               !PC->IsInputKeyDown(EKeys::RightMouseButton) && 
               !PC->IsInputKeyDown(EKeys::MiddleMouseButton))
            {
                float MouseX, MouseY;
                if (PC->GetMousePosition(MouseX, MouseY))
                {
                    int32 ViewportX, ViewportY;
                    PC->GetViewportSize(ViewportX, ViewportY);

                    float EdgeBorder = 25.0f; // Trigger if mouse is within 25 pixels of the edge
                    FVector2D PanInput(0.0f, 0.0f);

                    if (MouseX <= EdgeBorder) PanInput.Y = -1.0f; // Left
                    else if (MouseX >= ViewportX - EdgeBorder) PanInput.Y = 1.0f; // Right

                    if (MouseY <= EdgeBorder) PanInput.X = 1.0f; // Forward/Up
                    else if (MouseY >= ViewportY - EdgeBorder) PanInput.X = -1.0f; // Backward/Down

                    if (!PanInput.IsZero())
                    {
                        // Calculate movement relative to the camera's current orbit rotation
                        FRotator YawRot(0.0f, SpringArm->GetRelativeRotation().Yaw, 0.0f);
                        FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
                        FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

                        TargetLocation += (Forward * PanInput.X + Right * PanInput.Y) * EdgeScrollSpeed * DeltaTime;
                    }
                }
            }
        }
    }

    // 2. SMOOTH INTERPOLATION
    SetActorLocation(FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, 10.0f));
    SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetZoom, DeltaTime, 8.0f);
    SpringArm->SetRelativeRotation(FMath::RInterpTo(SpringArm->GetRelativeRotation(), TargetRotation, DeltaTime, 10.0f));
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

    // Calculate movement relative to the camera's current orbit rotation
    FRotator YawRot(0.0f, SpringArm->GetRelativeRotation().Yaw, 0.0f);
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
