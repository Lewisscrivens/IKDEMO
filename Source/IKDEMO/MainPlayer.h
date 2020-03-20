// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MainPlayer.generated.h"

/* Declare classes used. */
class USpringArmComponent;
class UCameraComponent;
class UInputComponent;

/* The IK player to demo IK tech for use in a game within Unreal Engine. */
UCLASS()
class IKDEMO_API AMainPlayer : public ACharacter
{
	GENERATED_BODY()

	/* Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/* Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

public:

	/* The camera movement mouse speed. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	float mouseSpeed;

	/* Last inputted direction for the character movement. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector lastDirectionMovement;

	/* The distance to check for the ground from the hips. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float groundCheckDistance;

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

public:

	/* Constructor. */
	AMainPlayer();

	/* Frame. */
	virtual void Tick(float DeltaTime) override;

	/* Gets the floor location and returns it in the world-axis. */
	FVector GetFloorLocation();

	/* Toggles the ragdoll on and off.
	 * NOTE: When ragdoll is toggled off, the character is reset and repositioned as it is static... */
	UFUNCTION(BlueprintCallable)
	void RagdollToggle();

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
