#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/WeaponAimDataAsset.h"
#include "MasterWeaponActor.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimSequence;
class UItemObject;

UENUM(BlueprintType)
enum class EWeaponAttachmentSlot : uint8
{
	Mag        UMETA(DisplayName="Mag"),
	Muzzle     UMETA(DisplayName="Muzzle/Silencer"),
	Scope      UMETA(DisplayName="Scope"),
	Grip       UMETA(DisplayName="Grip"),
	Laser      UMETA(DisplayName="Laser"),
	FlashLight UMETA(DisplayName="FlashLight"),
};

UCLASS()
class UESTALKER_API AMasterWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	AMasterWeaponActor();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	// ===== Components =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Components")
	TObjectPtr<USceneComponent> AttachmentsRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> MagMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> MuzzleMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> ScopeMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> GripMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> LaserMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> FlashLightMesh = nullptr;

	// ===== Default Meshes (настраиваешь в Details у конкретного оружия) =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Meshes")
	TObjectPtr<USkeletalMesh> DefaultWeapon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Meshes")
	TObjectPtr<USkeletalMesh> DefaultMag = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Meshes")
	TObjectPtr<USkeletalMesh> DefaultMuzzle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Meshes")
	TObjectPtr<USkeletalMesh> DefaultScope = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Meshes")
	TObjectPtr<USkeletalMesh> DefaultGrip = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Meshes")
	TObjectPtr<USkeletalMesh> DefaultLaser = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Meshes")
	TObjectPtr<USkeletalMesh> DefaultFlashLight = nullptr;

	// ===== Socket names (у каждого типа оружия свои) =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
	FName MagSocket = TEXT("mag");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
	FName MuzzleSocket = TEXT("muzzle");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
	FName ScopeSocket = TEXT("scope");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
	FName GripSocket = TEXT("grip");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
	FName LaserSocket = TEXT("laser");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
	FName FlashLightSocket = TEXT("flashlight");

	// Если задано — берём настройки отсюда
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	TObjectPtr<UWeaponAimDataAsset> AimData = nullptr;

	// Если AimData == nullptr — используем override
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(EditCondition="AimData==nullptr"))
	FWeaponAimSettings AimSettingsOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(EditCondition="AimData==nullptr"))
	TObjectPtr<UAnimSequence> AimAnimOverride = nullptr;

	// Вернёт настройки/анимацию/меш оружия (меш нужен чтобы читать AimSocket)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Aim")
	void GetAimData(FWeaponAimSettings& OutSettings, UAnimSequence*& OutAimAnim, USkeletalMeshComponent*& OutWeaponMesh) const;

	// ===== Reload (1P hands anim) =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload")
	TObjectPtr<UAnimSequence> ReloadAnimOverride = nullptr;

	// ===== Runtime link to item object =====
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Runtime")
	TObjectPtr<UItemObject> WeaponItemObject = nullptr;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Reload")
	void GetReloadAnim(UAnimSequence*& OutReloadAnim) const;

	/** Привязать runtime item (оружие) к актору, чтобы обновлять визуал магазина и читать кол-во патронов. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Runtime")
	void SetWeaponItemObject(UItemObject* InWeaponItemObject);

	/** Обновить визуал (например, показать/скрыть магазин) по текущему WeaponItemObject. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Runtime")
	void RefreshFromItemObject();

	UFUNCTION(BlueprintPure, Category="Weapon|Runtime")
	int32 GetAmmoInInsertedMag() const;

	UFUNCTION(BlueprintPure, Category="Weapon|Runtime")
	int32 GetInsertedMagCapacity() const;

	// ===== API =====
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetWeaponMesh(USkeletalMesh* NewMesh);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetAttachmentMesh(EWeaponAttachmentSlot Slot, USkeletalMesh* NewMesh);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void RefreshAttachmentSockets();
};