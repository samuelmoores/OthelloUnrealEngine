// Fill out your copyright notice in the Description page of Project Settings.

#include "GameCameraController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AGameCameraController::AGameCameraController()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SetRootComponent(SpringArm);
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
}

void AGameCameraController::BeginPlay()
{
	Super::BeginPlay();

	SpringArm->SetRelativeRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (InputMappingContext)
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}
}

void AGameCameraController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGameCameraController::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (OrbitAction)
		{
			EIC->BindAction(OrbitAction, ETriggerEvent::Triggered, this, &AGameCameraController::OnOrbit);
		}
	}
}

void AGameCameraController::OnOrbit(const FInputActionValue& Value)
{
	const FVector2D Delta = Value.Get<FVector2D>();

	CurrentYaw   += Delta.X * OrbitSensitivity;
	CurrentPitch  = FMath::Clamp(CurrentPitch - Delta.Y * OrbitSensitivity, PitchMin, PitchMax);

	SpringArm->SetRelativeRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));
}

