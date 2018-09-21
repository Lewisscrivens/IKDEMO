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

// Sets default values
AMainPlayer::AMainPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 400.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 400.0f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));

												// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	ragdollEnabled = false;
	jumpTimout = false;
	jumpTimer = 0.0f;
	jumpTimeToWait = 2.0f;
	jumpRotationSpeed = 0.1f;

	debugEnabled = true;
}

// Called when the game starts or when spawned
void AMainPlayer::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AMainPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Prevent player from jumping constantly.
	if (GetWorld()->GetRealTimeSeconds() > jumpTimer && jumpTimout)
	{
		jumpTimout = false;
	}

	if (ragdollEnabled)
	{
		// Move the current focus location to 40 above the root bone to keep track of player when in ragdoll.
		FVector rootBoneLocation = GetRootBoneLocation();
		rootBoneLocation.Z += 40.0f;// This fixes issues with the camera being stuck inside of the mesh at certain angles.
		CameraBoom->SetWorldLocation(rootBoneLocation);
	}

	// When the character is in air do not alow rotation towards movement.
	if (GetCharacterMovement()->IsFalling())
	{
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 0.0f, 0.0f);
	}
	else if (GetCharacterMovement()->RotationRate.IsZero())// Dont run code if the vector is not 0.
	{
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 400.0f, 0.0f);
	}
}


// Called to bind functionality to input
void AMainPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMainPlayer::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMainPlayer::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMainPlayer::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMainPlayer::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMainPlayer::LookUpAtRate);

	PlayerInputComponent->BindAction("Ragdoll", IE_Pressed, this, &AMainPlayer::RagdollToggle);
}

void AMainPlayer::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMainPlayer::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMainPlayer::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		lastDirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(lastDirectionForward, Value);
	}
}

void AMainPlayer::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		lastDirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(lastDirectionSideward, Value);
	}
}

void AMainPlayer::RagdollToggle()
{
	if (ragdollEnabled)
	{
		FVector newCapsuleLocation = CapsuleTrace();// Get the new location for the capsule in relation to where the physics body currently is.

		GetMesh()->SetSimulatePhysics(false);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);// Prevent the capsule from effectiong world objects while not in use.

		// Set the new location for the capsule.
		GetCapsuleComponent()->SetWorldLocation(newCapsuleLocation + FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
		// Re-attatch the camera spring arm component to the capsule. 
		CameraBoom->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		// The mesh needs reattatching to the capsule after entering ragdoll. Could be a bug with the engine version.
		GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -80.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

		ragdollEnabled = false;
	}
	else
	{
		// Enable ragdoll by simulating physics on the mesh and setting the new focus point for the spring arm as the root bone for the character mesh.
		GetMesh()->SetSimulatePhysics(true);	
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);// Prevent the capsule from effectiong world objects while not in use.

		CameraBoom->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);

		ragdollEnabled = true;
	}
}

void AMainPlayer::Jump()
{
	if (!jumpTimout)
	{
		Super::Jump();

		// If the character is running jump forward.
		if (!(GetCharacterMovement()->IsFalling()) && GetCharacterMovement()->Velocity.Size() > 0.0f)
		{
			GetCharacterMovement()->AddImpulse(GetDirectionToJump() * 50000.0f);
			currentJumpType = 1;
		}
		else
		{
			currentJumpType = 0;
		}

		// Initiate the timer for when the player can next jump.
		jumpTimer = GetWorld()->GetRealTimeSeconds() + jumpTimeToWait;
		jumpTimout = true;
	}
}

FVector AMainPlayer::GetDirectionToJump()
{
	// Create jump direction from where the camera is positioned in relation to the player.
	FVector directionToJump = GetCapsuleComponent()->GetForwardVector();
	directionToJump.Normalize();
	directionToJump.Z = 0.0f;

	return directionToJump;
}

// Used to find where to place the capsule when transitioning from ragdoll back to walking state. Returns the vector to place the capsule.
FVector AMainPlayer::CapsuleTrace()
{
	// Camera offset.
	float startDistance = 5.0f;

	// Line trace variable initialization.
	FHitResult hit;
	FVector Start;
	FVector End;
	FVector FoundFloor;
	FCollisionQueryParams TraceParams;
	TraceParams.bTraceComplex = true;

	//ignore the mesh when tracing from the root bone.
	TraceParams.AddIgnoredComponent(GetMesh());

	// Used to trace color in the line trace for debug.
	FColor lineTraceColour = FColor::Red;
	int lineTraceTimeLimit = 50; // How far the line trace will go.

	// Se the start and end distance to be from the characters root bone down along the z axis.
	Start = GetRootBoneLocation();
	End = Start + FVector(0.0f, 0.0f, -lineTraceTimeLimit);
	// Perform a single line trace.
	GetWorld()->LineTraceSingleByChannel(hit, Start, End, ECC_WorldStatic, TraceParams);

	// When the line hits the floor or something else within the trace channel.
	if (hit.bBlockingHit)
	{
		FoundFloor = hit.Location;
	}

	// Show debug lines for line trace.
	if (debugEnabled)
	{
		DrawDebugLine(GetWorld(), hit.TraceStart, hit.TraceEnd, lineTraceColour, false, lineTraceTimeLimit, 0.0f, 1.0f);
	}

	// Return the found floor location.
	return FoundFloor;
}

FVector AMainPlayer::GetRootBoneLocation()
{
	return GetMesh()->GetBoneLocation(FName("Base-HumanPelvis"), EBoneSpaces::WorldSpace);
}


/* No longer needed as you can no longer dive from standing still. Was in the tick function.
// If the character is falling. (When jumping or falling down from somewhere) rotate along the z-axis to face that direction.
if (GetCharacterMovement()->IsFalling())
{
FRotator Rot = FRotationMatrix::MakeFromX(GetDirectionToJump()).Rotator();// Get lookat rotation for direction the camera is looking.

GetCapsuleComponent()->AddWorldRotation(Rot * jumpRotationSpeed * DeltaTime);
}
*/