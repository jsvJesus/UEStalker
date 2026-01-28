#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/WeaponAimDataAsset.h"
#include "MasterWeaponActor.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimSequence;
class UAnimMontage;

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

	// ===== Animation =====
	/** Reload montage to play on 1P arms (Mesh1P AnimInstance) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage = nullptr;

	UFUNCTION(BlueprintPure, Category="Weapon|Animation")
	UAnimMontage* GetReloadMontage() const { return ReloadMontage; }

	/** Optional: reload montage to play on weapon mesh AnimInstance (for separate weapon anims) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<UAnimMontage> WeaponReloadMontage = nullptr;

	UFUNCTION(BlueprintPure, Category="Weapon|Animation")
	UAnimMontage* GetWeaponReloadMontage() const { return WeaponReloadMontage; }

	// ===== Socket names (у каждого типа оружия свои) =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
	FName MagSocket = TEXT("magazinSocket");

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

	// ===== Left hand IK targets (1P hands) =====
	// В обычной стойке левая рука цепляется к сокету/кости на WeaponMesh.
	// Если установлена рукоятка (GripMesh != nullptr && SkeletalMesh задан) -> берём LeftHandTarget_WithGrip.
	// Если рукоятки нет -> берём LeftHandTarget_NoGrip.
	// Во время перезарядки -> LeftHandTarget_ReloadMag (обычно кость магазина на WeaponMesh).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Hands")
	FName LeftHandTarget_WithGrip = TEXT("grip");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Hands")
	FName LeftHandTarget_NoGrip = TEXT("grip_none");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Hands")
	FName LeftHandTarget_ReloadMag = TEXT("magazinSocket");

	UFUNCTION(BlueprintPure, Category="Weapon|Hands")
	bool HasGripAttachment() const;

	UFUNCTION(BlueprintPure, Category="Weapon|Hands")
	FName GetLeftHandTargetName(bool bReloadMag) const;

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

	// ===== API =====
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetWeaponMesh(USkeletalMesh* NewMesh);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetAttachmentMesh(EWeaponAttachmentSlot Slot, USkeletalMesh* NewMesh);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void RefreshAttachmentSockets();
};