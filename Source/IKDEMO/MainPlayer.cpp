// Fill out your copyright notice in the Description page of Project Settings.
#include "MainPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Engine/GameEngine.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Runtime/Core/Public/Containers/Array.h"
#include "DrawDebugHelpers.h"
#include "IKAnimInstance.h"

AMainPlayer::AMainPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	// Setup player attachment component.
	playerHolder = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerHolder"));
	playerHolder->SetMobility(EComponentMobility::Movable);
	playerHolder->SetupAttachment(GetCapsuleComponent());
	GetMesh()->SetupAttachment(playerHolder);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -70.0f));

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(20.f, 75.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 400.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 400.0f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	camBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	camBoom->SetupAttachment(RootComponent);
	camBoom->TargetArmLength = 400.0f;
	camBoom->bUsePawnControlRotation = true;
	camBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));

	// Create a follow camera
	followCam = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	followCam->SetupAttachment(camBoom, USpringArmComponent::SocketName);
	followCam->bUsePawnControlRotation = false;

	// Setup default class variables.
	rootName = "Base-HumanPelvis";
	leftFootSocketName = "Base-HumanLFootSocket";
	rightFootSocketName = "Base-HumanRFootSocket";
	mouseSpeed = 45.f;
	ragdollEnabled = false;
	jumpRotationSpeed = 0.1f;
	debugEnabled = true;
	movementReleased = false;
	groundCheckDistance = 40.0f;
	defaultFloorDistance = 0.0f;
	hipOffset = 20.0f;
	ikUpdateRate = 0.01f;
	isIKEnabled = false;
	capsuleInterpSpeed = 7.0f;
	footTraceRadius = 5.0f;
}

void AMainPlayer::BeginPlay()
{
	Super::BeginPlay();

	// Setup IK update timer to be enabled by default.
	isIKEnabled = true;

	// Get default floor distance also.
	float hipsWorldZ = GetCapsuleComponent()->GetComponentLocation().Z;
	FVector leftFloorHit = GetFloorLocation(LEFT);
	defaultFloorDistance = FMath::Abs(hipsWorldZ - leftFloorHit.Z);

	// Get default relative offsets.
	FVector rightFloorHit = GetFloorLocation(RIGHT);
	FTransform capTrans = GetCapsuleComponent()->GetComponentTransform();
	leftRelativeFoot = capTrans.InverseTransformPositionNoScale(leftFloorHit);
	rightRelativeFoot = capTrans.InverseTransformPositionNoScale(rightFloorHit);

	// Save default capsule half height.
	capsuleOriginalHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	// Setup default feet positioning.
	UpdateDefaultFeetPosition();
}

void AMainPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If ragdoll is enabled update the camera boom location around the ragdoll.
	if (ragdollEnabled)
	{
		// Move the camera to its location based off the base human pelvis.
		FVector newCamLocation = GetMesh()->GetBoneLocation(rootName, EBoneSpaces::WorldSpace);
		newCamLocation -= originalOffset;
		camBoom->SetWorldLocation(newCamLocation);
	}

	// When the character is in air do not allow rotation towards movement.
	if (GetCharacterMovement()->IsFalling())
	{
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 0.0f, 0.0f);
	}
	// Dont run code if the vector is not 0.
	else if (GetCharacterMovement()->RotationRate.IsZero())
	{
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 400.0f, 0.0f);
	}

	// Get is moving.
	bool isMoving = InputComponent->GetAxisValue(FName("MoveForward")) != 0.0f || InputComponent->GetAxisValue(FName("MoveRight")) != 0.0f;

	// If all movement has stopped including release delay...
	if (movementReleased && !isMoving)
	{
		// Keep moving forward and ease out of movement. (Workaround)
		AddMovementInput(lastDirectionMovement, lastDirectionScale);
		GetCharacterMovement()->MaxWalkSpeed -= 1000 * DeltaTime;

		// End release delay when max walk speed becomes less than 0.
		if (GetCharacterMovement()->MaxWalkSpeed <= 0) movementReleased = false;
	}
	// Set movement back to normal.
	else GetCharacterMovement()->MaxWalkSpeed = 500.0f;

	// If IK is enabled update it.
	if (isIKEnabled && !GetCharacterMovement()->IsFalling() && !isMoving) UpdateIK();
	// Otherwise update default values.
	else UpdateDefaultFeetPosition();
}

void AMainPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings.
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMainPlayer::JumpAction<true>);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMainPlayer::JumpAction<false>);
	PlayerInputComponent->BindAction("Ragdoll", IE_Pressed, this, &AMainPlayer::RagdollToggle);

	// Set up axis bindings.
	PlayerInputComponent->BindAxis("MoveForward", this, &AMainPlayer::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMainPlayer::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMainPlayer::MouseX);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMainPlayer::MouseY);
}

void AMainPlayer::MouseX(float value)
{
	// Calculate and apply horizontal mouse movement on the camera.
	AddControllerYawInput(value * mouseSpeed * GetWorld()->GetDeltaSeconds());
}

void AMainPlayer::MouseY(float value)
{
	// Calculate and apply vertical mouse movement on the camera.
	AddControllerPitchInput(value * mouseSpeed * GetWorld()->GetDeltaSeconds());
}

void AMainPlayer::MoveForward(float value)
{
	// If there is not a controller exit. Or if the value is 0.
	if ((Controller != NULL) && (value != 0.0f))
	{
		// Find out which way is forward.
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get forward vector
		lastDirectionMovement = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		lastDirectionScale = value;

		// Add movement in that direction
		AddMovementInput(lastDirectionMovement, value);

		// Initiate slow down as soon as the value becomes 0...
		movementReleased = true;
	}
}

void AMainPlayer::MoveRight(float value)
{
	// If there is not a controller exit. Or if the value is 0.
	if ((Controller != NULL) && (value != 0.0f))
	{
		// Find out which way is right.
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get right vector 
		lastDirectionMovement = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		lastDirectionScale = value;

		// Add movement in that direction
		AddMovementInput(lastDirectionMovement, value);

		// Initiate slow down as soon as the value becomes 0...
		movementReleased = true;
	}
}

void AMainPlayer::RagdollToggle()
{
	if (ragdollEnabled)
	{
		// Get the new location for the capsule in relation to where the physics body currently is.
		FVector newCapsuleLocation = GetFloorLocation();

		// Reset mesh back to normal as static player character.
		GetMesh()->SetSimulatePhysics(false);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// Set the new location for the capsule and re-attach and position components.
		GetCapsuleComponent()->SetWorldLocation(newCapsuleLocation + FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
		camBoom->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		camBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -80.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}
	else
	{
		// Calculate camera offset to retain.
		originalOffset = GetMesh()->GetBoneTransform(GetMesh()->GetBoneIndex(rootName)).InverseTransformPositionNoScale(camBoom->GetComponentLocation());

		// Enable ragdoll by simulating physics on the mesh and setting the new focus point for the spring arm as the root bone for the character mesh.
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
		GetCharacterMovement()->DisableMovement();

		// Prevent the capsule from effecting world objects while not in use.
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		camBoom->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	}

	// Toggle ragdoll value.
	ragdollEnabled = !ragdollEnabled;
}

void AMainPlayer::ToggleIK(bool bEnable)
{
	isIKEnabled = bEnable;
}

void AMainPlayer::UpdateDefaultFeetPosition()
{
	// Get default positions for the left and right foot without any line traces in the world space.
	FTransform capTrans = GetCapsuleComponent()->GetComponentTransform();
	FVector currentLeftFoot = capTrans.TransformPositionNoScale(leftRelativeFoot);
	FVector currentRightFoot = capTrans.TransformPositionNoScale(rightRelativeFoot);

	// Update IKAnim.
	if (UIKAnimInstance* IKAnim = Cast<UIKAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		IKAnim->currentHipOffset = 0.0f;
		IKAnim->currentLeftFootLocation = currentLeftFoot;
		IKAnim->currentRightFootLocation = currentRightFoot;
	}
}

void AMainPlayer::UpdateIK()
{
	// Obtain the current foot offset in the Z direction for the left foot.
	FVector leftFloorHit = GetFloorLocation(LEFT);
	FVector rightFloorHit = GetFloorLocation(RIGHT);
	if ((leftFloorHit == FVector::ZeroVector || rightFloorHit == FVector::ZeroVector) && !ragdollEnabled)
	{
		// Toggle ragdoll and reset IK.
		RagdollToggle();
		UpdateDefaultFeetPosition();
		return;
	}
	
	// Get the IK offset values.
	FVector currLeftOffset = leftFloorHit;
	FVector currRightOffset = rightFloorHit;
	float bottomOfCapsuleZ = GetCapsuleComponent()->GetComponentLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float currHipOffset = currLeftOffset.Z < currRightOffset.Z ? FMath::Abs((FMath::Abs(currLeftOffset.Z) - FMath::Abs(bottomOfCapsuleZ))) * -1
															   : FMath::Abs((FMath::Abs(currRightOffset.Z) - FMath::Abs(bottomOfCapsuleZ))) * -1;

	// Update Capsule.
	UpdateCapsule(currHipOffset);

	// Create the correct offsets in the anim instance.
	if (UIKAnimInstance* IKAnim = Cast<UIKAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		IKAnim->currentHipOffset = currHipOffset;
		IKAnim->currentLeftFootLocation = currLeftOffset;
		IKAnim->currentRightFootLocation = currRightOffset;
	}
}

void AMainPlayer::UpdateCapsule(float offset, bool reset)
{
	// Get new half height.
	float newHeight = 0.0f;
	if (reset) newHeight = capsuleOriginalHeight;
	else newHeight = capsuleOriginalHeight - (FMath::Abs(offset) / 2);

	// Interpolate the value as long as this function is being ran.
	float currentCapsuleHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	float interpingValue = FMath::FInterpTo(currentCapsuleHeight, newHeight, GetWorld()->GetDeltaSeconds(), capsuleInterpSpeed);

	// Setup new capsule height.
	UCapsuleComponent* cap = GetCapsuleComponent();
	cap->SetCapsuleHalfHeight(interpingValue, true);

	// If debug is enabled draw the capsule.
	if (debugEnabled)
	{
		DrawDebugCapsule(GetWorld(), cap->GetComponentLocation(), cap->GetScaledCapsuleHalfHeight(), cap->GetScaledCapsuleRadius(), FQuat::Identity, FColor::Blue, false, 0.02f, 0.0f, 1.0f);
	}
}

void AMainPlayer::JumpAction(bool pressed)
{
	// Start and stop jumping when the jump input is pressed/released.
	if (pressed) Jump();
	else StopJumping();
}

void AMainPlayer::Jump()
{
	Super::Jump();

	// If the character is running jump forward.
	if (!GetCharacterMovement()->IsFalling() && GetCharacterMovement()->Velocity.Size() > 0.0f)
	{
		// Create jump direction from where the camera is positioned in relation to the player.
		FVector directionToJump = GetCapsuleComponent()->GetForwardVector();
		directionToJump.Normalize();
		directionToJump.Z = 0.0f;

		// Apply jump direction through force.
		GetCharacterMovement()->AddImpulse(directionToJump * 50000.0f);
	}
}

FVector AMainPlayer::GetFloorLocation(EGroundTraceType traceType)
{
	// Camera offset.
	float startDistance = 5.0f;

	// Line trace variable initialization.
	FHitResult hit;
	FVector startLoc = FVector::ZeroVector;
	FVector endLoc = FVector::ZeroVector;
	FVector floorLoc = FVector::ZeroVector;
	FCollisionQueryParams traceParams;
	traceParams.bTraceComplex = true;

	// Ignore this actor.
	traceParams.AddIgnoredActor(this);

	// Set the start of the trace depending on trace type.
	FTransform hipsTransform = GetCapsuleComponent()->GetComponentTransform();
	switch (traceType)
	{
	case CAPSULE:
		startLoc = GetMesh()->GetBoneLocation(rootName, EBoneSpaces::WorldSpace);
	break;
	case LEFT:
		startLoc = hipsTransform.TransformPositionNoScale(leftFootRelativeStart);
	break;
	case RIGHT:
		startLoc = hipsTransform.TransformPositionNoScale(rightFootRelativeStart);
	break;
	}
	
	// Set the end of the trace to be the ground check distance down in the world.
	endLoc = startLoc;
	endLoc.Z -= groundCheckDistance;

	// Perform a single line trace.
	GetWorld()->SweepSingleByChannel(hit, startLoc, endLoc, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(footTraceRadius), traceParams);
	if (hit.bBlockingHit) floorLoc = hit.Location;

	// Show debug lines for line trace.
	if (debugEnabled)
	{
		if (hit.bBlockingHit)
		{
			DrawDebugLine(GetWorld(), hit.TraceStart, hit.TraceEnd, FColor::Green, false, 0.2f, 0.0f, 0.5f);
			DrawDebugPoint(GetWorld(), hit.Location, 5.0f, FColor::Red, false, 0.2f, 0.0f);
		}
		else DrawDebugLine(GetWorld(), hit.TraceStart, hit.TraceEnd, FColor::Red, false, 0.2f, 0.0f, 0.5f);
	}

	// Return the found floor location.
	return floorLoc;
}