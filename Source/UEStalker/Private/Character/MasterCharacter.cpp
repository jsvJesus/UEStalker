#include "Character/MasterCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Components/InventoryComponent.h"
#include "Components/EquipmentComponent.h"
#include "Items/MasterWeaponActor.h"
#include "Animation/AnimInstance.h"
#include "Components/StaticMeshComponent.h"
#include "UI/HUD/GameHUDWidget.h"
#include "UI/Inventory/InventoryWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/MasterItemActor.h"
#include "Items/MasterItemDataAsset.h"
#include "Items/ItemObject.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Animation/AnimSequence.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

static void ConfigureHeldActorFor1P(AActor* Actor)
{
	if (!IsValid(Actor)) return;

	Actor->SetReplicates(false);
	Actor->SetActorEnableCollision(false);

	TArray<UActorComponent*> Comps;
	Actor->GetComponents(UPrimitiveComponent::StaticClass(), Comps);

	for (UActorComponent* C : Comps)
	{
		if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(C))
		{
			P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			P->SetGenerateOverlapEvents(false);
			P->SetOnlyOwnerSee(true);
			P->SetOwnerNoSee(false);
			P->CastShadow = false;
		}
	}
}

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

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = WalkSpeed;
	}

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FPSCamera);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	Mesh1P->bEnableUpdateRateOptimizations = false;
	Mesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh1P->PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	Mesh1P->AddTickPrerequisiteComponent(FPSCamera);

	// ===== Components X =====
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
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

	if (IsValid(EquipmentComponent))
	{
		EquipmentComponent->OnEquipmentSlotChanged.AddDynamic(this, &AMasterCharacter::OnEquipmentSlotChanged);

		if (IsValid(EquipmentComponent))
		{
			EquipmentComponent->SetSelectedSlot(ActiveWeaponSlot);
		}
	}

	Mesh1PHipRel = Mesh1P ? Mesh1P->GetRelativeTransform() : FTransform::Identity;
	DefaultFOV   = FPSCamera ? FPSCamera->FieldOfView : 90.f;

	// дефолтные aim settings, чтобы не было мусора
	CurrentAimSettings = FWeaponAimSettings();
	RefreshAimFromActiveWeapon();

	RebuildHeldActors();
	UpdateWeaponStateFromActiveSlot();
	UpdateWeaponVisuals();

	ApplyFirstPersonTickSync();
}

void AMasterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAim(DeltaSeconds);
	TickSyncedMontages();
}

void AMasterCharacter::StartAim()
{
	RefreshAimFromActiveWeapon();

	if (!CanAimNow())
	{
		bIsAiming = false;
		return;
	}

	bIsAiming = true;

	// Пересчёт цели прицеливания (по AimSocket)
	RebuildAimTargetFromSocket();
}

void AMasterCharacter::StopAim()
{
	bIsAiming = false;
}

void AMasterCharacter::Reload()
{
	if (!IsValid(Mesh1P))
	{
		return;
	}

	AMasterWeaponActor* WeaponActor = GetActiveWeaponActor();
	if (!IsValid(WeaponActor))
	{
		return;
	}

	UAnimMontage* ArmsMontage = WeaponActor->GetReloadMontage();
	if (!IsValid(ArmsMontage))
	{
		return;
	}

	UAnimInstance* ArmsAnim = Mesh1P->GetAnimInstance();
	if (!IsValid(ArmsAnim))
	{
		return;
	}

	if (ArmsAnim->Montage_IsPlaying(ArmsMontage))
	{
		return;
	}

	// На время перезарядки левая рука таргетится в магазин оружия
	SetLeftHandIKMode(ELeftHandIKMode::ReloadMag);

	UAnimMontage* WeaponMontage = WeaponActor->GetWeaponReloadMontage();
	if (IsValid(WeaponMontage))
	{
		StartSyncedMontages(ArmsMontage, WeaponMontage, 1.f);
	}
	else
	{
		StopSyncedMontages();
		ArmsAnim->Montage_Play(ArmsMontage, 1.f);
	}

	// Fallback: вернуть режим, когда перезарядка закончилась
	UAnimInstance* EndAnim = bSyncMontages ? SyncedArmsAnim.Get() : ArmsAnim;
	UAnimMontage* EndMontage = bSyncMontages ? SyncedArmsMontage.Get() : ArmsMontage;
	if (IsValid(EndAnim) && IsValid(EndMontage))
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AMasterCharacter::OnArmsReloadMontageEnded);
		EndAnim->Montage_SetEndDelegate(EndDelegate, EndMontage);
	}
}

void AMasterCharacter::UpdateAim(float DeltaSeconds)
{
	if (!IsValid(Mesh1P) || !IsValid(FPSCamera))
	{
		return;
	}

	const bool bTargetAim = bIsAiming && CanAimNow();
	const float TargetAlpha = bTargetAim ? 1.f : 0.f;

	// Если целимся — постоянно пересчитываем AimTarget по сокету.
	// Это нужно, чтобы при ходьбе (bob анимации) мушка оставалась в центре.
	if (bTargetAim)
	{
		RebuildAimTargetFromSocket();
	}

	// Alpha для AnimBP
	AimAlpha = FMath::FInterpTo(AimAlpha, TargetAlpha, DeltaSeconds, CurrentAimSettings.MeshInterpSpeed);

	// Целевой трансформ рук
	const FTransform TargetRel =
		(bTargetAim && bHasAimTarget) ? Mesh1PAimRel : Mesh1PHipRel;

	// Во время ADS на движении увеличиваем скорость подгона — меньше лаг/дрожь при bob анимациях
	float MeshInterpSpeed = CurrentAimSettings.MeshInterpSpeed;
	const float MoveSpeed2D = GetVelocity().Size2D();
	if (bTargetAim && MoveSpeed2D > 5.f)
	{
		MeshInterpSpeed = FMath::Max(MeshInterpSpeed, 60.f);
	}

	// Двигаем Mesh1P к цели
	if (bTargetAim && bHasAimTarget && AimAlpha > 0.98f)
	{
		// Полная фиксация в центре (без лага)
		Mesh1P->SetRelativeTransform(TargetRel, false, nullptr, ETeleportType::None);
	}
	else
	{
		const FVector CurLoc = Mesh1P->GetRelativeLocation();
		const FQuat   CurRot = Mesh1P->GetRelativeRotation().Quaternion();

		const FVector NewLoc = FMath::VInterpTo(CurLoc, TargetRel.GetLocation(), DeltaSeconds, MeshInterpSpeed);
		const FQuat   NewRot = FMath::QInterpTo(CurRot, TargetRel.GetRotation(), DeltaSeconds, MeshInterpSpeed);

		Mesh1P->SetRelativeLocationAndRotation(NewLoc, NewRot, false, nullptr, ETeleportType::None);
	}

	// FOV
	const float TargetFOV = bTargetAim ? CurrentAimSettings.AimFOV : DefaultFOV;
	const float NewFOV = FMath::FInterpTo(FPSCamera->FieldOfView, TargetFOV, DeltaSeconds, CurrentAimSettings.FOVInterpSpeed);
	FPSCamera->SetFieldOfView(NewFOV);
}

void AMasterCharacter::RefreshAimFromActiveWeapon()
{
	CurrentAimSettings = FWeaponAimSettings();
	CurrentAimAnim = nullptr;

	TObjectPtr<AActor> ActiveActor = nullptr;
	if (const TObjectPtr<AActor>* Ptr = GetHeldActorPtrBySlot(ActiveWeaponSlot))
	{
		ActiveActor = *Ptr;
	}

	if (const AMasterWeaponActor* WeaponActor = Cast<AMasterWeaponActor>(ActiveActor))
	{
		FWeaponAimSettings NewSettings;
		UAnimSequence* NewAnim = nullptr;
		USkeletalMeshComponent* WeaponMesh = nullptr;

		WeaponActor->GetAimData(NewSettings, NewAnim, WeaponMesh);

		CurrentAimSettings = NewSettings;
		CurrentAimAnim = NewAnim;
	}
}

void AMasterCharacter::RebuildAimTargetFromSocket()
{
	bHasAimTarget = false;

	if (!IsValid(Mesh1P) || !IsValid(FPSCamera))
	{
		return;
	}

	USkeletalMeshComponent* WeaponMesh = GetActiveWeaponMesh();
	if (!IsValid(WeaponMesh))
	{
		return;
	}

	const FName SocketName = CurrentAimSettings.AimSocketName;
	if (!WeaponMesh->DoesSocketExist(SocketName))
	{
		return;
	}

	// Текущий "hip" берём как базу для расчёта (чтобы не было скачков)
	const FTransform BaseMeshRel = Mesh1P->GetRelativeTransform();

	const FTransform CamWorld = FPSCamera->GetComponentTransform();
	const FTransform SocketWorld = WeaponMesh->GetSocketTransform(SocketName, RTS_World);

	// Позиция/поворот сокета в пространстве камеры
	const FVector SocketLocCam = CamWorld.InverseTransformPosition(SocketWorld.GetLocation());
	const FQuat   SocketRotCam = CamWorld.GetRotation().Inverse() * SocketWorld.GetRotation();

	// Хотим, чтобы AimSocket оказался в (0,0,0) камеры и совпал по ротации с камерой
	const FVector TargetLoc = BaseMeshRel.GetLocation()
		- SocketLocCam
		+ CurrentAimSettings.AdditionalOffset;

	const FQuat TargetRot =
		(FQuat(CurrentAimSettings.AdditionalRotation) *
		SocketRotCam.Inverse() *
		BaseMeshRel.GetRotation()).GetNormalized();

	// Защита от «улёта» оружия: если сокет/меш кривой, не двигаем Mesh1P слишком далеко
	const float MaxReasonableDelta = 200.f; // см
	if ((TargetLoc - BaseMeshRel.GetLocation()).Size() > MaxReasonableDelta)
	{
		return;
	}

	Mesh1PAimRel = FTransform(TargetRot, TargetLoc, BaseMeshRel.GetScale3D());
	bHasAimTarget = true;
}

USkeletalMeshComponent* AMasterCharacter::GetActiveWeaponMesh() const
{
	// Берём actor активного слота
	TObjectPtr<AActor> ActiveActor = nullptr;

	if (const TObjectPtr<AActor>* Ptr = GetHeldActorPtrBySlot(ActiveWeaponSlot))
	{
		ActiveActor = *Ptr;
	}

	if (!IsValid(ActiveActor))
	{
		return nullptr;
	}

	// Если это MasterWeaponActor — берём его WeaponMesh через GetAimData (там и socket)
	if (const AMasterWeaponActor* WeaponActor = Cast<AMasterWeaponActor>(ActiveActor))
	{
		FWeaponAimSettings Tmp;
		UAnimSequence* TmpAnim = nullptr;
		USkeletalMeshComponent* WeaponMesh = nullptr;
		WeaponActor->GetAimData(Tmp, TmpAnim, WeaponMesh);
		return WeaponMesh;
	}

	// Фоллбек: первый SkeletalMeshComponent на акторе
	return ActiveActor->FindComponentByClass<USkeletalMeshComponent>();
}

bool AMasterCharacter::CanAimNow() const
{
	if (bIsSprinting)
	{
		return false;
	}

	// Разреши целиться только когда реально оружие
	if (CurrentWeaponState != EWeaponState::Weapon_AK &&
		CurrentWeaponState != EWeaponState::Weapon_Pistol)
	{
		return false;
	}

	return CurrentAimSettings.bEnableADS;
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

		// Sprinting
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started,   this, &AMasterCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMasterCharacter::StopSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled,  this, &AMasterCharacter::StopSprint);
		}

		// Open/Close Inventory
		EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AMasterCharacter::ToggleInventory);

		// Pickup (F)
		if (PickupAction)
		{
			EnhancedInputComponent->BindAction(PickupAction, ETriggerEvent::Started, this, &AMasterCharacter::PickupItem);
		}

		// Selected Weapon
		if (SelectPrimaryWeaponAction)
			EnhancedInputComponent->BindAction(SelectPrimaryWeaponAction, ETriggerEvent::Started, this, &AMasterCharacter::SelectPrimaryWeaponSlot);

		if (SelectSecondaryWeaponAction)
			EnhancedInputComponent->BindAction(SelectSecondaryWeaponAction, ETriggerEvent::Started, this, &AMasterCharacter::SelectSecondaryWeaponSlot);

		if (SelectPistolAction)
			EnhancedInputComponent->BindAction(SelectPistolAction, ETriggerEvent::Started, this, &AMasterCharacter::SelectPistolSlot);

		if (SelectKnifeAction)
			EnhancedInputComponent->BindAction(SelectKnifeAction, ETriggerEvent::Started, this, &AMasterCharacter::SelectKnifeSlot);

		if (SelectGrenadePrimaryAction)
			EnhancedInputComponent->BindAction(SelectGrenadePrimaryAction, ETriggerEvent::Started, this, &AMasterCharacter::SelectGrenadePrimarySlot);

		if (SelectGrenadeSecondaryAction)
			EnhancedInputComponent->BindAction(SelectGrenadeSecondaryAction, ETriggerEvent::Started, this, &AMasterCharacter::SelectGrenadeSecondarySlot);

		// Aim Weapon
		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started,   this, &AMasterCharacter::StartAim);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AMasterCharacter::StopAim);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled,  this, &AMasterCharacter::StopAim);
		}

		// Reload Weapon
		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AMasterCharacter::Reload);
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

void AMasterCharacter::SetActiveWeaponSlot(EEquipmentSlotId NewSlot)
{
	const bool bAllowed =
		NewSlot == EEquipmentSlotId::PrimaryWeaponSlot ||
		NewSlot == EEquipmentSlotId::SecondaryWeaponSlot ||
		NewSlot == EEquipmentSlotId::PistolSlot ||
		NewSlot == EEquipmentSlotId::KnifeSlot ||
		NewSlot == EEquipmentSlotId::GrenadePrimarySlot ||
		NewSlot == EEquipmentSlotId::GrenadeSecondarySlot;

	if (!bAllowed)
	{
		return;
	}

	// Toggle: нажали ту же кнопку ещё раз -> убрали оружие
	if (ActiveWeaponSlot == NewSlot && CurrentWeaponState != EWeaponState::Unarmed)
	{
		ActiveWeaponSlot = NewSlot;
		CurrentWeaponState = EWeaponState::Unarmed;
		if (IsValid(EquipmentComponent))
		{
			EquipmentComponent->SetSelectedSlot(EEquipmentSlotId::None);
		}
		UpdateWeaponVisuals();
		return;
	}

	ActiveWeaponSlot = NewSlot;

	// UI highlight (и вообще “выбранный слот”)
	if (IsValid(EquipmentComponent))
	{
		EquipmentComponent->SetSelectedSlot(NewSlot);
	}

	UpdateWeaponStateFromActiveSlot();
	UpdateWeaponVisuals();
}

void AMasterCharacter::SetWeaponState(EWeaponState NewState)
{
	CurrentWeaponState = NewState;

	// если включили оружейную стойку — выбираем слот где реально есть оружие
	if (CurrentWeaponState == EWeaponState::Unarmed && IsValid(EquipmentComponent))
	{
		EquipmentComponent->SetSelectedSlot(EEquipmentSlotId::None);
	}

	UpdateWeaponVisuals();
}

void AMasterCharacter::SelectPrimaryWeaponSlot()
{
	SetActiveWeaponSlot(EEquipmentSlotId::PrimaryWeaponSlot);
}

void AMasterCharacter::SelectSecondaryWeaponSlot()
{
	SetActiveWeaponSlot(EEquipmentSlotId::SecondaryWeaponSlot);
}

void AMasterCharacter::SelectPistolSlot()
{
	SetActiveWeaponSlot(EEquipmentSlotId::PistolSlot);
}

void AMasterCharacter::SelectKnifeSlot()
{
	SetActiveWeaponSlot(EEquipmentSlotId::KnifeSlot);
}

void AMasterCharacter::SelectGrenadePrimarySlot()
{
	SetActiveWeaponSlot(EEquipmentSlotId::GrenadePrimarySlot);
}

void AMasterCharacter::SelectGrenadeSecondarySlot()
{
	SetActiveWeaponSlot(EEquipmentSlotId::GrenadeSecondarySlot);
}

void AMasterCharacter::StartSprint()
{
	if (bIsSprinting)
	{
		return;
	}

	bIsSprinting = true;

	// Sprint отменяет Aim
	StopAim();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = SprintSpeed;
	}
}

void AMasterCharacter::StopSprint()
{
	if (!bIsSprinting)
	{
		return;
	}

	bIsSprinting = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = WalkSpeed;
	}
}

void AMasterCharacter::ApplyFirstPersonTickSync()
{
	if (!IsValid(Mesh1P) || !IsValid(FPSCamera))
	{
		return;
	}
	
	// Mesh1P must always evaluate every frame; no URO; tick after camera
	Mesh1P->bEnableUpdateRateOptimizations = false;
	Mesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh1P->PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	Mesh1P->AddTickPrerequisiteComponent(FPSCamera);
	
	ApplyTickSyncToHeldActor(HeldPrimaryActor);
	ApplyTickSyncToHeldActor(HeldSecondaryActor);
	ApplyTickSyncToHeldActor(HeldPistolActor);
	ApplyTickSyncToHeldActor(HeldKnifeActor);
	ApplyTickSyncToHeldActor(HeldGrenadePrimaryActor);
	ApplyTickSyncToHeldActor(HeldGrenadeSecondaryActor);
}

void AMasterCharacter::ApplyTickSyncToHeldActor(AActor* HeldActor)
{
	if (!IsValid(HeldActor) || !IsValid(Mesh1P))
	{
		return;
	}
	
	TArray<USkeletalMeshComponent*> Skels;
	HeldActor->GetComponents<USkeletalMeshComponent>(Skels);
	
	for (USkeletalMeshComponent* Skel : Skels)
	{
		if (!IsValid(Skel))
		{
			continue;
		}
			
		Skel->bEnableUpdateRateOptimizations = false;
		Skel->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Skel->PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
			
		// Critical: weapon/attachments tick after hands, otherwise you get 1-frame lag ("рассинхрон")
		Skel->AddTickPrerequisiteComponent(Mesh1P);
	}
}

void AMasterCharacter::StartSyncedMontages(UAnimMontage* ArmsMontage, UAnimMontage* WeaponMontage, float PlayRate)
{
	StopSyncedMontages();
	
	if (!IsValid(Mesh1P) || !IsValid(ArmsMontage))
		{
			return;
		}
	
	UAnimInstance* ArmsAnim = Mesh1P->GetAnimInstance();
	if (!IsValid(ArmsAnim))
	{
		return;
	}
	
	ArmsAnim->Montage_Play(ArmsMontage, PlayRate);
	
	if (!IsValid(WeaponMontage))
	{
		return;
	}
	
	USkeletalMeshComponent* WeaponMesh = GetActiveWeaponMesh();
	UAnimInstance* WeaponAnim = IsValid(WeaponMesh) ? WeaponMesh->GetAnimInstance() : nullptr;
	if (!IsValid(WeaponAnim))
	{
		return;
	}
	
	WeaponAnim->Montage_Play(WeaponMontage, PlayRate);
	
	// Align start times
	const float ArmsPos = ArmsAnim->Montage_GetPosition(ArmsMontage);
	WeaponAnim->Montage_SetPosition(WeaponMontage, ArmsPos);
	
	SyncedArmsAnim = ArmsAnim;
	SyncedWeaponAnim = WeaponAnim;
	SyncedArmsMontage = ArmsMontage;
	SyncedWeaponMontage = WeaponMontage;
	bSyncMontages = true;

	if (LeftHandIKMode == ELeftHandIKMode::ReloadMag)
	{
		LeftHandIKMode = ELeftHandIKMode::Grip;
	}
}

void AMasterCharacter::TickSyncedMontages()
{
	if (!bSyncMontages)
	{
		return;
	}
	
	UAnimInstance* ArmsAnim = SyncedArmsAnim.Get();
	UAnimInstance* WeaponAnim = SyncedWeaponAnim.Get();
	UAnimMontage* ArmsMontage = SyncedArmsMontage.Get();
	UAnimMontage* WeaponMontage = SyncedWeaponMontage.Get();
	
	if (!IsValid(ArmsAnim) || !IsValid(WeaponAnim) || !IsValid(ArmsMontage) || !IsValid(WeaponMontage))
	{
		StopSyncedMontages();
		return;
	}
	
	if (!ArmsAnim->Montage_IsPlaying(ArmsMontage) || !WeaponAnim->Montage_IsPlaying(WeaponMontage))
	{
		StopSyncedMontages();
		return;
	}
	
	const float ArmsPos = ArmsAnim->Montage_GetPosition(ArmsMontage);
	WeaponAnim->Montage_SetPosition(WeaponMontage, ArmsPos);
	
	// keep playrate consistent (optional)
	const float ArmsRate = ArmsAnim->Montage_GetPlayRate(ArmsMontage);
	WeaponAnim->Montage_SetPlayRate(WeaponMontage, ArmsRate);
}

void AMasterCharacter::StopSyncedMontages()
{
	bSyncMontages = false;
	SyncedArmsAnim = nullptr;
	SyncedWeaponAnim = nullptr;
	SyncedArmsMontage = nullptr;
	SyncedWeaponMontage = nullptr;
}

void AMasterCharacter::SetLeftHandIKMode(ELeftHandIKMode NewMode)
{
	LeftHandIKMode = NewMode;
}

AMasterWeaponActor* AMasterCharacter::GetActiveWeaponActor() const
{
	const TObjectPtr<AActor>* HeldPtr = GetHeldActorPtrBySlot(ActiveWeaponSlot);
	AActor* Held = HeldPtr ? HeldPtr->Get() : nullptr;
	return Cast<AMasterWeaponActor>(Held);
}

void AMasterCharacter::OnArmsReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (LeftHandIKMode == ELeftHandIKMode::ReloadMag)
	{
		LeftHandIKMode = ELeftHandIKMode::Grip;
	}
}

void AMasterCharacter::OnEquipmentSlotChanged(EEquipmentSlotId SlotId, UItemObject* Item)
{
	// Перестраиваем визуал для слотов, которые могут быть в руках
	if (SlotId == EEquipmentSlotId::PrimaryWeaponSlot ||
		SlotId == EEquipmentSlotId::SecondaryWeaponSlot ||
		SlotId == EEquipmentSlotId::PistolSlot ||
		SlotId == EEquipmentSlotId::KnifeSlot ||
		SlotId == EEquipmentSlotId::GrenadePrimarySlot ||
		SlotId == EEquipmentSlotId::GrenadeSecondarySlot)
	{
		RebuildHeldActors();
	}

	// обновить стойку/визуал (если сняли оружие, и т.п.)
	UpdateWeaponStateFromActiveSlot();
	UpdateWeaponVisuals();
}

void AMasterCharacter::RebuildHeldActors()
{
	StopSyncedMontages();
	
	DestroyHeldActorSafe(HeldPrimaryActor);
	DestroyHeldActorSafe(HeldSecondaryActor);
	DestroyHeldActorSafe(HeldPistolActor);
	DestroyHeldActorSafe(HeldKnifeActor);
	DestroyHeldActorSafe(HeldGrenadePrimaryActor);
	DestroyHeldActorSafe(HeldGrenadeSecondaryActor);

	if (!IsValid(EquipmentComponent))
	{
		return;
	}

	HeldPrimaryActor = SpawnHeldActorFromItem(EquipmentComponent->GetItemInSlot(EEquipmentSlotId::PrimaryWeaponSlot));
	HeldSecondaryActor = SpawnHeldActorFromItem(EquipmentComponent->GetItemInSlot(EEquipmentSlotId::SecondaryWeaponSlot));
	HeldPistolActor = SpawnHeldActorFromItem(EquipmentComponent->GetItemInSlot(EEquipmentSlotId::PistolSlot));
	HeldKnifeActor = SpawnHeldActorFromItem(EquipmentComponent->GetItemInSlot(EEquipmentSlotId::KnifeSlot));
	HeldGrenadePrimaryActor = SpawnHeldActorFromItem(EquipmentComponent->GetItemInSlot(EEquipmentSlotId::GrenadePrimarySlot));
	HeldGrenadeSecondaryActor = SpawnHeldActorFromItem(EquipmentComponent->GetItemInSlot(EEquipmentSlotId::GrenadeSecondarySlot));
}

void AMasterCharacter::UpdateWeaponStateFromActiveSlot()
{
	if (!IsValid(EquipmentComponent))
	{
		CurrentWeaponState = EWeaponState::Unarmed;
		return;
	}

	UItemObject* ActiveItem = EquipmentComponent->GetItemInSlot(ActiveWeaponSlot);
	CurrentWeaponState = DeriveWeaponStateFromItem(ActiveItem);

	// если в выбранном слоте пусто/ничего в руках -> снимаем выделение
	if (CurrentWeaponState == EWeaponState::Unarmed && IsValid(EquipmentComponent))
	{
		EquipmentComponent->SetSelectedSlot(EEquipmentSlotId::None);
	}

	if (CurrentWeaponState == EWeaponState::Unarmed ||
	CurrentWeaponState == EWeaponState::Grenade ||
	CurrentWeaponState == EWeaponState::Grenade_Bolt ||
	CurrentWeaponState == EWeaponState::Weapon_Knife)
	{
		StopAim();
	}
}

void AMasterCharacter::UpdateWeaponVisuals()
{
	// скрываем всё
	SetHeldActorVisible(HeldPrimaryActor, false);
	SetHeldActorVisible(HeldSecondaryActor, false);
	SetHeldActorVisible(HeldPistolActor, false);
	SetHeldActorVisible(HeldKnifeActor, false);
	SetHeldActorVisible(HeldGrenadePrimaryActor, false);
	SetHeldActorVisible(HeldGrenadeSecondaryActor, false);

	if (CurrentWeaponState == EWeaponState::Unarmed)
	{
		ApplyFirstPersonTickSync();
		LeftHandIKMode = ELeftHandIKMode::None;
		return;
	}
	
	if (LeftHandIKMode == ELeftHandIKMode::None)
    {
    	LeftHandIKMode = ELeftHandIKMode::Grip;
    }

	// показать актор активного слота (если ещё не создан — создадим)
	if (TObjectPtr<AActor>* Ptr = GetHeldActorPtrBySlot(ActiveWeaponSlot))
	{
		if (!IsValid(*Ptr) && IsValid(EquipmentComponent))
		{
			*Ptr = SpawnHeldActorFromItem(EquipmentComponent->GetItemInSlot(ActiveWeaponSlot));
		}
		SetHeldActorVisible(*Ptr, true);
	}

	RefreshAimFromActiveWeapon();

	if (bIsAiming)
	{
		RebuildAimTargetFromSocket();
	}

	ApplyFirstPersonTickSync();
}

EWeaponState AMasterCharacter::DeriveWeaponStateFromItem(const UItemObject* Item) const
{
	if (!IsValid(Item))
	{
		return EWeaponState::Unarmed;
	}

	// Явный override из DataAsset (ItemDetails)
	if (Item->ItemDetails.HandsWeaponState != EWeaponState::Unarmed)
	{
		return Item->ItemDetails.HandsWeaponState;
	}

	// Болт/гранаты
	if (Item->ItemDetails.ItemSubCategory == EItemSubCategory::ItemSubCat_Weapons_Grenade)
	{
		switch (Item->ItemDetails.GrenadeType)
		{
		case EGrenadeType::Grenade_Bolt:
			return EWeaponState::Grenade_Bolt;
		case EGrenadeType::Grenade_Frag:
			return EWeaponState::Grenade;
		default:
			return EWeaponState::Grenade;
		}
	}

	// Нож
	if (Item->ItemDetails.ItemSubCategory == EItemSubCategory::ItemSubCat_Weapons_Knife)
	{
		return EWeaponState::Weapon_Knife;
	}

	// Пистолет
	if (Item->ItemDetails.ItemSubCategory == EItemSubCategory::ItemSubCat_Weapons_HG)
	{
		return EWeaponState::Weapon_Pistol;
	}

	// Оружие
	if (Item->ItemDetails.ItemCategory == EItemCategory::ItemCat_Weapons)
	{
		switch (Item->ItemDetails.WeaponType)
		{
		case EWeaponType::Weapon_AK12:
		case EWeaponType::Weapon_AK74:
			return EWeaponState::Weapon_AK;

		default:
			// Fallback: всё что не HG/Knife/Grenade считаем "автоматной" стойкой
			return EWeaponState::Weapon_AK;
		}
	}

	return EWeaponState::Unarmed;
}

AActor* AMasterCharacter::SpawnHeldActorFromItem(UItemObject* Item)
{
	if (!IsValid(Item)) return nullptr;
	UWorld* W = GetWorld();
	if (!IsValid(W)) return nullptr;

	// приоритет: HandsClass (DataAsset)
	TSubclassOf<AActor> ClassToSpawn = Item->ItemDetails.HandsClass;

	// если не задано — пробуем ItemClass, но только если это НЕ pickup-актор
	if (!ClassToSpawn && Item->ItemDetails.ItemClass &&
		!Item->ItemDetails.ItemClass->IsChildOf(AMasterItemActor::StaticClass()))
	{
		ClassToSpawn = Item->ItemDetails.ItemClass;
	}

	// fallback на дефолтный WeaponActor для любого оружия
	if (!ClassToSpawn && Item->ItemDetails.ItemCategory == EItemCategory::ItemCat_Weapons)
	{
		ClassToSpawn = DefaultWeaponActorClass;
	}

	if (!ClassToSpawn)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* A = W->SpawnActor<AActor>(ClassToSpawn, FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!IsValid(A))
	{
		return nullptr;
	}

	ConfigureHeldActorFor1P(A);

	// attach к сокету
	if (IsValid(Mesh1P))
	{
		const FName Socket = Item->ItemDetails.HandsSocket.IsNone() ? WeaponAttachSocketName : Item->ItemDetails.HandsSocket;

		if (!Socket.IsNone() && !Mesh1P->DoesSocketExist(Socket))
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("[HeldActor] Socket '%s' does not exist on Mesh1P for item '%s'"),
				*Socket.ToString(), *GetNameSafe(Item->SourceAsset));
		}

		const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
		A->AttachToComponent(Mesh1P, Rules, Socket);
	}

	// Ensure weapon/attachments tick after Mesh1P (fixes 1-frame lag / desync)
	ApplyTickSyncToHeldActor(A);

	SetHeldActorVisible(A, false);
	return A;
}

void AMasterCharacter::DestroyHeldActorSafe(TObjectPtr<AActor>& ActorPtr)
{
	if (IsValid(ActorPtr))
	{
		ActorPtr->Destroy();
	}
	ActorPtr = nullptr;
}

void AMasterCharacter::SetHeldActorVisible(AActor* Actor, bool bVisible)
{
	if (!IsValid(Actor)) return;

	Actor->SetActorHiddenInGame(!bVisible);
	Actor->SetActorEnableCollision(false);

	TArray<UActorComponent*> Comps;
	Actor->GetComponents(UPrimitiveComponent::StaticClass(), Comps);
	for (UActorComponent* C : Comps)
	{
		if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(C))
		{
			P->SetHiddenInGame(!bVisible);
			P->SetVisibility(bVisible, true);
		}
	}
}

TObjectPtr<AActor>* AMasterCharacter::GetHeldActorPtrBySlot(EEquipmentSlotId SlotId)
{
	switch (SlotId)
	{
	case EEquipmentSlotId::PrimaryWeaponSlot: return &HeldPrimaryActor;
	case EEquipmentSlotId::SecondaryWeaponSlot: return &HeldSecondaryActor;
	case EEquipmentSlotId::PistolSlot: return &HeldPistolActor;
	case EEquipmentSlotId::KnifeSlot: return &HeldKnifeActor;
	case EEquipmentSlotId::GrenadePrimarySlot: return &HeldGrenadePrimaryActor;
	case EEquipmentSlotId::GrenadeSecondarySlot: return &HeldGrenadeSecondaryActor;
	default: break;
	}
	return nullptr;
}

const TObjectPtr<AActor>* AMasterCharacter::GetHeldActorPtrBySlot(EEquipmentSlotId SlotId) const
{
	switch (SlotId)
	{
	case EEquipmentSlotId::PrimaryWeaponSlot:   return &HeldPrimaryActor;
	case EEquipmentSlotId::SecondaryWeaponSlot: return &HeldSecondaryActor;
	case EEquipmentSlotId::PistolSlot:          return &HeldPistolActor;
	case EEquipmentSlotId::KnifeSlot:           return &HeldKnifeActor;
	case EEquipmentSlotId::GrenadePrimarySlot:  return &HeldGrenadePrimaryActor;
	case EEquipmentSlotId::GrenadeSecondarySlot:return &HeldGrenadeSecondaryActor;
	default: break;
	}
	return nullptr;
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

void AMasterCharacter::PickupItem()
{
	if (!IsValid(InventoryComponent))
	{
		return;
	}

	// Пока открыт инвентарь — не подбираем
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
	const float UnitWeight = FMath::Max(0.f, Asset->ItemDetails.ItemWeight);

	bool bStackChanged = false;

	auto CalcAllowedByWeight = [&](int32 Want) -> int32
	{
		if (Want <= 0) return 0;

		// Если веса нет или лимит отключен — не ограничиваем
		if (UnitWeight <= KINDA_SMALL_NUMBER || InventoryComponent->GetMaxCarryWeight() <= 0.f)
		{
			return Want;
		}

		const float Free = InventoryComponent->GetMaxCarryWeight() - InventoryComponent->GetTotalWeight();
		if (Free <= 0.f)
		{
			return 0;
		}

		const int32 ByWeight = FMath::FloorToInt(Free / UnitWeight);
		return FMath::Clamp(ByWeight, 0, Want);
	};

	// Докидываем в существующие стаки
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

			int32 Add = FMath::Min(Remaining, Space);

			// Ограничиваем по весу
			Add = CalcAllowedByWeight(Add);
			if (Add <= 0)
			{
				// веса больше нет — дальше бессмысленно
				break;
			}

			Existing->SetStackCount(Curr + Add);
			bStackChanged = true;

			PickedUp += Add;
			Remaining -= Add;

			if (Remaining <= 0)
			{
				break;
			}
		}
	}

	if (bStackChanged)
	{
		InventoryComponent->MarkInventoryChanged();
	}

	// Остаток — создаём новые ItemObject и пытаемся добавить
	while (Remaining > 0)
	{
		int32 Chunk = bStackable ? FMath::Min(Remaining, MaxStack) : 1;

		// Ограничиваем по весу (иначе TryAddItem просто вернет false и мы зря создадим объект)
		Chunk = CalcAllowedByWeight(Chunk);
		if (Chunk <= 0)
		{
			break;
		}

		UItemObject* NewItem = NewObject<UItemObject>(InventoryComponent);
		if (!IsValid(NewItem))
		{
			break;
		}

		NewItem->InitializeFromAsset(Asset, Chunk);

		if (!InventoryComponent->TryAddItem(NewItem))
		{
			// Нет места или перегруз — прекращаем (остаток остается на земле)
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
