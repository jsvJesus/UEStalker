#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Items/MasterItemStructs.h"
#include "ItemObject.generated.h"

class UMasterItemDataAsset;
class USoundBase;

/**
 * Runtime-экземпляр предмета (в инвентаре).
 * Тут CurrDurability/CurrCharge/Stack/Rotation и т.п.
 */
UCLASS(BlueprintType)
class UItemObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	TObjectPtr<UMasterItemDataAsset> SourceAsset = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FMasterItemDetails ItemDetails;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FItemTradeConfig TradeConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FItemDurabilityConfig DurabilityConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FItemChargeConfig ChargeConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FItemOutfitStatsConfig OutfitStatsConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FItemWeaponsStatsConfig WeaponStatsConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FItemMagazineConfig MagazineConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FConsumablesStats ConsumablesStats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime")
	FItemRuntimeState Runtime;

	// =========================================
	// Weapon/Magazine runtime (attachments/ammo)
	// =========================================

	/** Если этот предмет — оружие, сюда кладём вставленный магазин (UItemObject магазина). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Runtime")
	TObjectPtr<UItemObject> InsertedMagazine = nullptr;

	/** Если этот предмет — магазин, ссылка на оружие в котором он сейчас стоит (чтобы нельзя было вставить в два места). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Magazine|Runtime")
	TObjectPtr<UItemObject> OwnerWeapon = nullptr;

	/** Текущее кол-во патронов в магазине (может быть 0). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Magazine|Runtime", meta=(ClampMin="0", UIMin="0"))
	int32 MagazineAmmoCount = 0;

	/** Тип патрона который сейчас загружен в магазин (None если магазин пуст). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Magazine|Runtime")
	EAmmoType MagazineLoadedAmmoType = EAmmoType::AmmoType_None;

	/** Референс на DataAsset патронов которые загружены (нужно для веса/иконки; NULL если магазин пуст). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Magazine|Runtime")
	TObjectPtr<UMasterItemDataAsset> MagazineLoadedAmmoAsset = nullptr;

public:
	UFUNCTION(BlueprintCallable, Category="Item")
	void InitializeFromAsset(UMasterItemDataAsset* InAsset, int32 InStackCount = 1);

	UFUNCTION(BlueprintCallable, Category="Item")
	void SetStackCount(int32 NewCount);

	UFUNCTION(BlueprintPure, Category="Item")
	bool IsStackable() const { return ItemDetails.bIsStackable; }

	UFUNCTION(BlueprintPure, Category="Item")
	int32 GetMaxStack() const { return ItemDetails.bIsStackable ? ItemDetails.MaxStackCount : 1; }

	UFUNCTION(BlueprintPure, Category="Item", meta=(DisplayName="Get Sound Of Use"))
	USoundBase* GetSoundOfUse() const;

	UFUNCTION(BlueprintPure, Category="Item", meta=(DisplayName="Get Dimensions"))
	void GetDimensions(FItemSize& Dimensions) const;

	UFUNCTION(BlueprintPure, Category="Item", meta=(DisplayName="Get Equipment Sound"))
	USoundBase* GetEquipmentSound() const;

	UFUNCTION(BlueprintPure, Category="Item", meta=(DisplayName="Get Drop Sound"))
	USoundBase* GetDropSound() const;

	UFUNCTION(BlueprintPure, Category="Item", meta=(DisplayName="Get Pickup Sound"))
	USoundBase* GetPickupSound() const;

	UFUNCTION(BlueprintPure, Category="Item", meta=(DisplayName="Get Outfit Stats"))
	FItemOutfitStatsConfig GetOutfitStats() const;

	// =========================================
	// Type helpers
	// =========================================

	UFUNCTION(BlueprintPure, Category="Item|Type")
	bool IsWeapon() const;

	UFUNCTION(BlueprintPure, Category="Item|Type")
	bool IsMagazine() const;

	UFUNCTION(BlueprintPure, Category="Item|Type")
	bool IsAmmo() const;

	// =========================================
	// Magazine helpers
	// =========================================

	UFUNCTION(BlueprintPure, Category="Item|Magazine")
	int32 GetMagazineCapacity() const;

	UFUNCTION(BlueprintPure, Category="Item|Magazine")
	int32 GetMagazineAmmo() const { return FMath::Max(0, MagazineAmmoCount); }

	UFUNCTION(BlueprintPure, Category="Item|Magazine")
	EAmmoType GetMagazineLoadedAmmoType() const { return MagazineLoadedAmmoType; }

	/** Текст "0/30" для UMG. */
	UFUNCTION(BlueprintPure, Category="Item|Magazine")
	FText GetMagazineAmmoText() const;

	// =========================================
	// Ammo helpers
	// =========================================

	/** Текст "120" (stack). */
	UFUNCTION(BlueprintPure, Category="Item|Ammo")
	FText GetAmmoCountText() const;
};