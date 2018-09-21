// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MainPlayer.generated.h"

UCLASS()
class IKDEMO_API AMainPlayer : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

public:
	// Sets default values for this character's properties
	AMainPlayer();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseLookUpRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	FVector lastDirectionForward;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	FVector lastDirectionSideward;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	bool ragdollEnabled;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	bool jumpTimout;

	// Used in AnimBP to check what type of jump is being performed.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	int currentJumpType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float jumpTimeToWait;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float jumpTimer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float jumpRotationSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Debug)
	bool debugEnabled;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/**
	* Called via input to turn at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void TurnAtRate(float Rate);

	/**
	* Called via input to turn look up/down at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void LookUpAtRate(float Rate);

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void RagdollToggle();

	void Jump() override;

	FVector GetDirectionToJump();

	FVector CapsuleTrace();

	FVector GetRootBoneLocation();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
};