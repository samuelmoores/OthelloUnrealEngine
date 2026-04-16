// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "InputActionValue.h"
#include "GameCameraController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class OTHELLO_UNREALENGINE_API AGameCameraController : public APawn
{
	GENERATED_BODY()

public:
	AGameCameraController();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* Camera;

	// Assign these in the Blueprint's Class Defaults
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* OrbitAction;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float OrbitSensitivity = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float PitchMin = -80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float PitchMax = -10.0f;

private:
	float CurrentYaw = 0.0f;
	float CurrentPitch = -45.0f;

	void OnOrbit(const FInputActionValue& Value);

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
