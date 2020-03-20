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
#include "Components/SkeletalMeshComponent.h"
#include "Runtime/Core/Public/Containers/Array.h"
#include "DrawDebugHelpers.h"

AMainPlayer::AMainPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

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
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Setup default class variables.
	mouseSpeed = 45.f;
	ragdollEnabled = false;
	jumpRotationSpeed = 0.1f;
	debugEnabled = true;
	movementReleased = false;
	groundCheckDistance = 40.0f;
}

void AMainPlayer::BeginPlay()
{
	Super::BeginPlay();

	//....
}

void AMainPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If ragdoll is enabled update the camera boom location around the ragdoll.
	if (ragdollEnabled)
	{
		// Move the camera to its location based off the base human pelvis.
		FVector newCamLocation = GetMesh()->GetBoneLocation("Base-HumanPelvis", EBoneSpaces::WorldSpace);
		newCamLocation -= originalOffset;
		CameraBoom->SetWorldLocation(newCamLocation);
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

	// If all movement has stopped including release delay...
	if (movementReleased && InputComponent->GetAxisValue(FName("MoveForward")) == 0.0f && InputComponent->GetAxisValue(FName("MoveRight")) == 0.0f)
	{
		// Keep moving forward and ease out of movement. (Workaround)
		AddMovementInput(lastDirectionMovement, lastDirectionScale);
		GetCharacterMovement()->MaxWalkSpeed -= 1000 * DeltaTime;

		// End release delay when max walk speed becomes less than 0.
		if (GetCharacterMovement()->MaxWalkSpeed <= 0) movementReleased = false;
	}
	// Set movement back to normal.
	else GetCharacterMovement()->MaxWalkSpeed = 500.0f;
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
		CameraBoom->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -80.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}
	else
	{
		// Calculate camera offset to retain.
		originalOffset = GetMesh()->GetBoneTransform(GetMesh()->GetBoneIndex("Base-HumanPelvis")).InverseTransformPositionNoScale(CameraBoom->GetComponentLocation());

		// Enable ragdoll by simulating physics on the mesh and setting the new focus point for the spring arm as the root bone for the character mesh.
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
		GetCharacterMovement()->DisableMovement();

		// Prevent the capsule from effecting world objects while not in use.
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CameraBoom->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	}

	// Toggle ragdoll value.
	ragdollEnabled = !ragdollEnabled;
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

FVector AMainPlayer::GetFloorLocation()
{
	// Camera offset.
	float startDistance = 5.0f;

	// Line trace variable initialization.
	FHitResult hit;
	FVector startLoc;
	FVector endLoc;
	FVector floorLoc;
	FCollisionQueryParams traceParams;
	traceParams.bTraceComplex = true;

	// Ignore the mesh when tracing from the root bone.
	traceParams.AddIgnoredComponent(GetMesh());

	// Set the start and end distance to be from the characters root bone down along the z axis.
	startLoc = GetMesh()->GetBoneLocation("Base-HumanPelvis", EBoneSpaces::WorldSpace);
	endLoc = startLoc + FVector(0.0f, 0.0f, -groundCheckDistance);

	// Perform a single line trace.
	GetWorld()->LineTraceSingleByChannel(hit, startLoc, endLoc, ECC_WorldStatic, traceParams);
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