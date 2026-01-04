#pragma once

#include "CoreMinimal.h"
#include "Items/MasterItemEnums.h"
#include "Engine/Texture2D.h"
#include "Engine/SkeletalMesh.h"
#include "Sound/SoundBase.h"
#include "MasterItemStructs.generated.h"

USTRUCT(BlueprintType)
struct FTile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tile")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tile")
	int32 Y = 0;
};

USTRUCT(BlueprintType)
struct FLine2D
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line")
	FVector2D Start = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line")
	FVector2D End = FVector2D::ZeroVector;
};

/** ItemSize (размер предмета в клетках инвентаря) */
USTRUCT(BlueprintType)
struct FItemSize
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ItemSize", meta=(ClampMin="1", UIMin="1"))
	int32 X = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ItemSize", meta=(ClampMin="1", UIMin="1"))
	int32 Y = 1;

	FORCEINLINE FIntPoint ToIntPoint() const { return FIntPoint(X, Y); }
};

USTRUCT(BlueprintType)
struct FItemColorsCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Condition")
	TObjectPtr<UTexture2D> BaseCondition = nullptr;   // 0..15% (Gray)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Condition")
	TObjectPtr<UTexture2D> GoodCondition = nullptr;   // 15..50% (White)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Condition")
	TObjectPtr<UTexture2D> AverageCondition = nullptr; // 50..80% (Orange)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Condition")
	TObjectPtr<UTexture2D> PoorCondition = nullptr;   // 80..100% (Red)
};

/** Buy/Sell (bHasInSell/bHasInBuy + цены) */
USTRUCT(BlueprintType)
struct FItemTradeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trade")
	bool bCanSell = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trade")
	bool bCanBuy = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trade", meta=(ClampMin="0.0", UIMin="0.0"))
	float ItemCostSell = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trade", meta=(ClampMin="0.0", UIMin="0.0"))
	float ItemCostBuy = 0.f;
};

/** Прочность (конфиг) */
USTRUCT(BlueprintType)
struct FItemDurabilityConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Durability")
	bool bHasDurability = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Durability", meta=(EditCondition="bHasDurability", ClampMin="0.0", UIMin="0.0"))
	float MaxDurability = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Durability", meta=(EditCondition="bHasDurability"))
	bool bCanRepair = false;
};

/** Заряд (конфиг) */
USTRUCT(BlueprintType)
struct FItemChargeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge")
	bool bHasCharge = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge", meta=(EditCondition="bHasCharge"))
	bool bUsesBattery = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge", meta=(EditCondition="bHasCharge"))
	bool bCanCharge = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge", meta=(EditCondition="bHasCharge", ClampMin="0.0", UIMin="0.0"))
	float MaxCharge = 0.f;
};

/** Рантайм состояние (CurrDurability/CurrCharge/Stack/Rotated) */
USTRUCT(BlueprintType)
struct FItemRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime")
	int32 StackCount = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime")
	float CurrDurability = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime")
	float CurrCharge = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime")
	bool bIsRotated = false;
};

/** Защиты (Outfit) */
USTRUCT(BlueprintType)
struct FItemProtectionStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Protection")
	float Radiation = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Protection")
	float Psi = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Protection")
	float Chemical = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Protection")
	float Blow = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Protection")
	float Electrical = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Protection")
	float Bullets = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Protection")
	float Fire = 0.f;
};

/** OutfitStatsConfig (одежда/броня/шлем/рюкзак/hero parts) */
USTRUCT(BlueprintType)
struct FItemOutfitStatsConfig
{
	GENERATED_BODY()

	// HeroParts / Outfit meshes
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meshes|HeroParts")
	TObjectPtr<USkeletalMesh> MeshHands = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meshes|HeroParts")
	TObjectPtr<USkeletalMesh> MeshFPSHands = nullptr;

	/** По дефолту null: одежда может менять полный Mesh_Body (без головы/рук FPS/рук) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meshes|Outfit")
	TObjectPtr<USkeletalMesh> MeshClothBody = nullptr;

	// Gear meshes
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meshes|Gear")
	TObjectPtr<USkeletalMesh> MeshHelmet = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meshes|Gear")
	TObjectPtr<USkeletalMesh> MeshArmor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meshes|Gear")
	TObjectPtr<USkeletalMesh> MeshBackpack = nullptr;

	/** Научные костюмы: можно скрывать визуал шлема/броника/рюкзака */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meshes|Overrides")
	bool bHideHelmetMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meshes|Overrides")
	bool bHideArmorMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meshes|Overrides")
	bool bHideBackpackMesh = false;

	// Carry weights
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carry", meta=(ClampMin="0.0", UIMin="0.0"))
	float CarryBonusWeight = 0.f; // бонус от рюкзака и т.п.

	// Protections
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Protection")
	FItemProtectionStats Protection;
};

/** WeaponStatsConfig (минимум — дальше расширишь) */
USTRUCT(BlueprintType)
struct FItemWeaponsStatsConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	EAmmoType AmmoType = EAmmoType::AmmoType_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0", UIMin="0"))
	int32 MagazineSize = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0.0", UIMin="0.0"))
	float Damage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0.0", UIMin="0.0"))
	float FireRateRPM = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0.0", UIMin="0.0"))
	float ReloadTime = 0.f;
};

/** ConsumablesStats (еда/вода/мед) */
USTRUCT(BlueprintType)
struct FConsumablesStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Consumable")
	float Health = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Consumable")
	float Stamina = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Consumable")
	float Hunger = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Consumable")
	float Thirst = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Consumable")
	float Weight = 0.f;
};

/**
 * ItemDetails (ТОЛЬКО “что это за предмет”: айди, имена, иконки, размер, звуки, стак и т.п.)
 * ВАЖНО: сюда НЕ кладем защиты/урон/бафы — это в отдельных конфигах.
 */
USTRUCT(BlueprintType)
struct FMasterItemDetails
{
	GENERATED_BODY()

	// IDs / categories
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Details")
	int32 ItemID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Details")
	EItemCategory ItemCategory = EItemCategory::ItemCat_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Details")
	EItemSubCategory ItemSubCategory = EItemSubCategory::ItemSubCat_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Details")
	EAmmoType AmmoType = EAmmoType::AmmoType_None; // актуально если ItemCat_Ammo или оружие

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Details")
	EItemFilter ItemFilter = EItemFilter::Filter_None;

	// Class
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Details")
	TSubclassOf<AActor> ItemClass = nullptr;

	// Names (указатели/ключи под БД)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Text")
	FName ItemDisplayName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Text")
	FName ItemName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Text")
	FName ItemDesc = NAME_None;

	// Stack
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stack")
	bool bIsStackable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stack", meta=(EditCondition="bIsStackable", ClampMin="1", UIMin="1"))
	int32 MaxStackCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stack", meta=(EditCondition="bIsStackable", ClampMin="1", UIMin="1"))
	int32 MinStackCount = 1;

	/** Если не стак — сколько лежит по умолчанию (например патроны пачкой) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stack", meta=(ClampMin="1", UIMin="1"))
	int32 DefaultCount = 1;

	// Weight
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0.0", UIMin="0.0"))
	float ItemWeight = 0.f;

	// UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TObjectPtr<UTexture2D> ItemIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TObjectPtr<UTexture2D> IconRotated = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	bool bCanRotate = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	FItemSize Size; // 1x1, 2x2, ...

	// Sounds
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sounds")
	TObjectPtr<USoundBase> ItemDropSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sounds")
	TObjectPtr<USoundBase> ItemEquipSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sounds")
	TObjectPtr<USoundBase> ItemUseSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sounds")
	TObjectPtr<USoundBase> ItemPickupSound = nullptr;

	// Condition UI (опционально)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Condition")
	FItemColorsCondition ColorsCondition;
};