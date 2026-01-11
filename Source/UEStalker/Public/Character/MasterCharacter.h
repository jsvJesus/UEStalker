#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Components/EquipmentComponent.h"
#include "Items/WeaponAimDataAsset.h"
#include "MasterCharacter.generated.h"

enum class EEquipmentSlotId : uint8;
class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInventoryComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
class UGameHUDWidget;
class UInventoryWidget;
class AMasterItemActor;
class AMasterWeaponActor;
class UStaticMeshComponent;
class UItemObject;
class UAnimSequence;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS()
class UESTALKER_API AMasterCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FPSCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* InventoryAction;

	/** Pickup / Interact Input Action (Key: F) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* PickupAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* SelectPrimaryWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* SelectSecondaryWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* SelectPistolAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* SelectKnifeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* SelectGrenadePrimaryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* SelectGrenadeSecondaryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* AimAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* SprintAction = nullptr;

public:
	AMasterCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void StartAim();
	void StopAim();

	void UpdateAim(float DeltaSeconds);
	bool CanAimNow() const;

	void RefreshAimFromActiveWeapon();
	USkeletalMeshComponent* GetActiveWeaponMesh() const;
	void RebuildAimTargetFromSocket();

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	// APawn interface
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	UFUNCTION()
	void ToggleInventory();

	UFUNCTION()
	void PickupItem();

	/** Инвентарь изменился (в т.ч. когда магазин/патроны применены через Drag&Drop) */
	UFUNCTION()
	void OnInventoryChangedEvent();

	AMasterItemActor* GetBestPickupCandidate() const;

	void ApplyInventoryInputMode(bool bOpen);

public:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
	TObjectPtr<UEquipmentComponent> EquipmentComponent;

	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FPSCamera; }

	// HUD
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UGameHUDWidget> GameHUDWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category="UI")
	TObjectPtr<UGameHUDWidget> GameHUDWidget = nullptr;

	// Inventory
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category="UI")
	TObjectPtr<UInventoryWidget> InventoryWidget = nullptr;

	// Weapons State
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName WeaponAttachSocketName = TEXT("weapon_r");

	// Если ItemDetails.ItemClass не задан/не AMasterWeaponActor — fallback на этот класс
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TSubclassOf<AMasterWeaponActor> DefaultWeaponActorClass;
	
	// Спавны в руках по слотам
	UPROPERTY(Transient, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<AActor> HeldPrimaryActor = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<AActor> HeldSecondaryActor = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<AActor> HeldPistolActor = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<AActor> HeldKnifeActor = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<AActor> HeldGrenadePrimaryActor = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<AActor> HeldGrenadeSecondaryActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	EEquipmentSlotId ActiveWeaponSlot = EEquipmentSlotId::PrimaryWeaponSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	EWeaponState CurrentWeaponState = EWeaponState::Unarmed;

	UFUNCTION(BlueprintPure, Category="Weapon")
	EWeaponState GetWeaponState() const { return CurrentWeaponState; }

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetActiveWeaponSlot(EEquipmentSlotId NewSlot);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetWeaponState(EWeaponState NewState);

	UFUNCTION()
	void SelectPrimaryWeaponSlot();

	UFUNCTION()
	void SelectSecondaryWeaponSlot();

	UFUNCTION()
	void SelectPistolSlot();

	UFUNCTION()
	void SelectKnifeSlot();

	UFUNCTION()
	void SelectGrenadePrimarySlot();

	UFUNCTION()
	void SelectGrenadeSecondarySlot();

	// Sprint settings
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Sprint", meta=(AllowPrivateAccess="true"))
	float WalkSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Sprint", meta=(AllowPrivateAccess="true"))
	float SprintSpeed = 600.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement|Sprint", meta=(AllowPrivateAccess="true"))
	bool bIsSprinting = false;

	UFUNCTION()
	void StartSprint();

	UFUNCTION()
	void StopSprint();

protected:
	UFUNCTION()
	void OnEquipmentSlotChanged(EEquipmentSlotId SlotId, UItemObject* Item);

	void RebuildHeldActors();
	void UpdateWeaponStateFromActiveSlot();
	void UpdateWeaponVisuals();

	EWeaponState DeriveWeaponStateFromItem(const UItemObject* Item) const;

	AActor* SpawnHeldActorFromItem(UItemObject* Item);
	void DestroyHeldActorSafe(TObjectPtr<AActor>& ActorPtr);
	void SetHeldActorVisible(AActor* Actor, bool bVisible);

	TObjectPtr<AActor>* GetHeldActorPtrBySlot(EEquipmentSlotId SlotId);
	const TObjectPtr<AActor>* GetHeldActorPtrBySlot(EEquipmentSlotId SlotId) const;

public:
	// AIM state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Aim")
	bool bIsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Aim")
	float AimAlpha = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Aim")
	TObjectPtr<UAnimSequence> CurrentAimAnim = nullptr;

	FWeaponAimSettings CurrentAimSettings;

	// Кэш трансформов рук (Mesh1P) для hip/aim
	FTransform Mesh1PHipRel;
	FTransform Mesh1PAimRel;
	bool bHasAimTarget = false;

	// FOV
	float DefaultFOV = 90.f;

	// API для AnimInstance
	UFUNCTION(BlueprintPure, Category="Weapon|Aim")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category="Weapon|Aim")
	float GetAimAlpha() const { return AimAlpha; }

	UFUNCTION(BlueprintPure, Category="Weapon|Aim")
	UAnimSequence* GetAimSequence() const { return CurrentAimAnim; }
};
