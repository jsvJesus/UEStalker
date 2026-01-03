#include "Character/MasterCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Components/InventoryComponent.h"
#include "UI/HUD/GameHUDWidget.h"
#include "UI/Inventory/InventoryWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AMasterCharacter::AMasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FPSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FPSCamera->SetupAttachment(GetCapsuleComponent());
	FPSCamera->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FPSCamera->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FPSCamera);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	// ===== InventoryComponent =====
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}

void AMasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!IsValid(PC))
	{
		return;
	}

	if (IsValid(GameHUDWidgetClass))
	{
		GameHUDWidget = CreateWidget<UGameHUDWidget>(PC, GameHUDWidgetClass);
		if (IsValid(GameHUDWidget))
		{
			GameHUDWidget->AddToViewport(0);
		}
	}

	if (IsValid(InventoryWidgetClass))
	{
		InventoryWidget = CreateWidget<UInventoryWidget>(PC, InventoryWidgetClass);
		if (IsValid(InventoryWidget))
		{
			InventoryWidget->AddToViewport(10);
			InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
			
			InventoryWidget->InitializeWidget(InventoryComponent, 64.f);

			ApplyInventoryInputMode(false);
		}
	}
}

void AMasterCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AMasterCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AMasterCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AMasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMasterCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMasterCharacter::Look);

		// Open/Close Inventory
		EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AMasterCharacter::ToggleInventory);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMasterCharacter::ToggleInventory()
{
	if (!IsValid(InventoryWidget))
	{
		return;
	}

	const bool bIsOpen = (InventoryWidget->GetVisibility() != ESlateVisibility::Hidden);
	const bool bOpenNow = !bIsOpen;

	InventoryWidget->SetVisibility(bOpenNow ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	ApplyInventoryInputMode(bOpenNow);
}

void AMasterCharacter::ApplyInventoryInputMode(bool bOpen)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!IsValid(PC))
	{
		return;
	}

	PC->SetShowMouseCursor(bOpen);

	if (bOpen)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

		if (IsValid(InventoryWidget))
		{
			Mode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		}

		PC->SetInputMode(Mode);
	}
	else
	{
		FInputModeGameOnly Mode;
		PC->SetInputMode(Mode);
	}
}
