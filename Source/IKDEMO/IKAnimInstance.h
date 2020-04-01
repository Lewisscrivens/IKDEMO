// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "IKAnimInstance.generated.h"

/* IK anim instance class to hold some C++ updated variables for the MainPlayer class. */
UCLASS()
class IKDEMO_API UIKAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:

	/* Constructor. */
	UIKAnimInstance();

public:

	/* The current world location of the left foot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector currentLeftFootLocation;

	/* The current world location of the right foot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector currentRightFootLocation;

	/* The amount to offset the hips. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float currentHipOffset;
};
