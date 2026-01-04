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
#include "Items/ItemObject.h"
#include "Items/MasterItemActor.h"
#include "Items/MasterItemDataAsset.h"
#include "Kismet/GameplayStatics.h"

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

		// Pickup (F)
		if (PickupAction)
		{
			EnhancedInputComponent->BindAction(PickupAction, ETriggerEvent::Started, this, &AMasterCharacter::PickupItem);
		}
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

void AMasterCharacter::PickupItem()
{
	if (!IsValid(InventoryComponent))
	{
		return;
	}

	// Пока открыт инвентарь — не подбираем (чтобы не ловить случайные нажатия)
	if (IsValid(InventoryWidget) && InventoryWidget->GetVisibility() != ESlateVisibility::Hidden)
	{
		return;
	}

	AMasterItemActor* ItemActor = GetBestPickupCandidate();
	if (!IsValid(ItemActor) || !IsValid(ItemActor->ItemData))
	{
		return;
	}

	UMasterItemDataAsset* Asset = ItemActor->ItemData;

	int32 Remaining = FMath::Max(1, ItemActor->WorldStackCount);
	int32 PickedUp = 0;

	const bool bStackable = Asset->ItemDetails.bIsStackable;
	const int32 MaxStack = FMath::Max(1, Asset->ItemDetails.MaxStackCount);

	// Сначала докидываем в существующие стаки (если стакуемое)
	if (bStackable && Remaining > 0)
	{
		const TArray<UItemObject*> AllItems = InventoryComponent->GetAllItems();
		for (UItemObject* Existing : AllItems)
		{
			if (!IsValid(Existing) || Existing->SourceAsset != Asset || !Existing->IsStackable())
			{
				continue;
			}

			const int32 Curr = FMath::Max(1, Existing->Runtime.StackCount);
			const int32 Space = FMath::Max(0, MaxStack - Curr);
			if (Space <= 0)
			{
				continue;
			}

			const int32 Add = FMath::Min(Remaining, Space);
			Existing->SetStackCount(Curr + Add);
			PickedUp += Add;
			Remaining -= Add;

			if (Remaining <= 0)
			{
				break;
			}
		}
	}

	// Остаток — создаем новые ItemObject и раскладываем по сетке
	while (Remaining > 0)
	{
		const int32 Chunk = bStackable ? FMath::Min(Remaining, MaxStack) : 1;

		UItemObject* NewItem = NewObject<UItemObject>(InventoryComponent);
		if (!IsValid(NewItem))
		{
			break;
		}

		NewItem->InitializeFromAsset(Asset, Chunk);

		if (!InventoryComponent->TryAddItem(NewItem))
		{
			// Нет места — прекращаем (остаток остается на земле)
			break;
		}

		PickedUp += Chunk;
		Remaining -= Chunk;
	}

	if (PickedUp <= 0)
	{
		return;
	}

	// Звук подбора
	if (USoundBase* PickupSound = Asset->ItemDetails.ItemPickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, ItemActor->GetActorLocation());
	}

	// Обновляем/уничтожаем предмет в мире
	if (Remaining <= 0)
	{
		ItemActor->Destroy();
	}
	else
	{
		ItemActor->WorldStackCount = Remaining;
	}
}

AMasterItemActor* AMasterCharacter::GetBestPickupCandidate() const
{
	TArray<AActor*> Overlapping;
	GetOverlappingActors(Overlapping, AMasterItemActor::StaticClass());

	const FVector RefLoc = IsValid(FPSCamera) ? FPSCamera->GetComponentLocation() : GetActorLocation();

	AMasterItemActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AActor* A : Overlapping)
	{
		AMasterItemActor* ItemActor = Cast<AMasterItemActor>(A);
		if (!IsValid(ItemActor) || !IsValid(ItemActor->ItemData))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(RefLoc, ItemActor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = ItemActor;
		}
	}

	return Best;
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
