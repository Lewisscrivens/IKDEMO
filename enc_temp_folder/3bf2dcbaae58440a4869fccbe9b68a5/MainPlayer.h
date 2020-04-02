// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MainPlayer.generated.h"

/* Declare classes used. */
class USpringArmComponent;
class UCameraComponent;
class UInputComponent;

/* Enum to change what the GetFloorLocation() function does. */
UENUM(BlueprintType)
enum EGroundTraceType
{
	CAPSULE,
	LEFT,
	RIGHT
};

/* The IK player to demo IK tech for use in a game within Unreal Engine. */
UCLASS()
class IKDEMO_API AMainPlayer : public ACharacter
{
	GENERATED_BODY()

	/* Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* camBoom;

	/* Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* followCam;

	/* Player Holder for default offset to the capsule... */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* playerHolder;

public:

	/* The name of the root bone. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|IK")
	FName rootName;

	/* The name of the left foot socket. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|IK")
	FName leftFootSocketName;

	/* The name of the right foot socket. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|IK")
	FName rightFootSocketName;

	/* The camera movement mouse speed. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	float mouseSpeed;

	/* Last inputted direction for the character movement. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector lastDirectionMovement;

	/* The distance to check for the ground from the hips. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|IK")
	float groundCheckDistance;

	/* The radius of the foot trace for detecting the floor for IK adjustments of the feet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|IK")
	float footTraceRadius;

	/* Offset to check from the hips for either leg. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|IK")
	float hipOffset;

	/* Offset to check from the hips for either leg. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|IK")
	float ikUpdateRate;

	/* Speed to interp capsule IK offset in height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|IK")
	float capsuleInterpSpeed;

	/* The left relative offset to trace from for the feet relative to the hips. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|IK")
	FVector leftFootRelativeStart;
	
	/* The right relative offset to trace from for the feet relative to the hips. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|IK")
	FVector rightFootRelativeStart;

	/* Is ragdoll enabled? */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool ragdollEnabled;

	/* Rotation acceleration. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float jumpRotationSpeed;

	/* Debug properties within the class to the log? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool debugEnabled;

	/* Is there any movement input keyboard-wise from the player? */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement")
	bool movementReleased;

private:

	float lastDirectionScale; /* The last direction along the movement axis from player input. */
	float defaultFloorDistance; /* The expected distance from the hips world Z to the ground on a flat surface. */
	float capsuleOriginalHeight; /* The original capsule half height. */
	FVector originalOffset; /* Camera offset when rag dolling to avoid snapping movement of the camera... */
	FTimerHandle ikTimer; /* The timer handle for the UpdateIK function to stop the timer at runtime. */
	bool isIKEnabled; /* Is IK currently active? */
	

public:

	/* Constructor. */
	AMainPlayer();

	/* Frame. */
	virtual void Tick(float DeltaTime) override;

	/* Gets the floor location and returns it in the world-axis. */
	FVector GetFloorLocation(EGroundTraceType type = CAPSULE);

	/* Toggles the ragdoll on and off.
	 * NOTE: When ragdoll is toggled off, the character is reset and repositioned as it is static... */
	UFUNCTION(BlueprintCallable)
	void RagdollToggle();

	/* Toggles the IK on or off depending on given bEnable value. */
	UFUNCTION(BlueprintCallable)
	void ToggleIK(bool bEnable);

	/* IK update function. */
	UFUNCTION(Category = "IK")
	void UpdateIK();

	/* Updates the capsule size depending on IK offset value and can also reset the capsule back to normal. */
	UFUNCTION(BlueprintCallable, Category = "IK")
	void UpdateCapsule(float offset = 0.0f, bool reset = false);

protected:

	/* Level start. */
	virtual void BeginPlay() override;

	/* Input constructor. */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/* Forward movement input function. */
	UFUNCTION(Category = "Movement")
	void MoveForward(float value);

	/* Right movement input function. */
	UFUNCTION(Category = "Movement")
	void MoveRight(float value);

	/* The current movement from the mouse along the x axis. */
	UFUNCTION(Category = "Movement")
	void MouseX(float value);

	/* The current movement from the mouse along the y axis. */
	UFUNCTION(Category = "Movement")
	void MouseY(float value);

	/* Jump input function. */
	template<bool val> void JumpAction() { JumpAction(val); }
	UFUNCTION(Category = "Movement")
	void JumpAction(bool pressed);

	/* Jump function. */
	void Jump() override;
};
