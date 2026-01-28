#include "Items/ItemObject.h"
#include "Items/MasterItemDataAsset.h"
#include "Sound/SoundBase.h"

void UItemObject::InitializeFromAsset(UMasterItemDataAsset* InAsset, int32 InStackCount)
{
	SourceAsset = InAsset;

	if (!SourceAsset)
	{
		ItemDetails = FMasterItemDetails{};
		TradeConfig = FItemTradeConfig{};
		DurabilityConfig = FItemDurabilityConfig{};
		ChargeConfig = FItemChargeConfig{};
		OutfitStatsConfig = FItemOutfitStatsConfig{};
		WeaponStatsConfig = FItemWeaponsStatsConfig{};
		MagazineConfig = FItemMagazineConfig{};
		ConsumablesStats = FConsumablesStats{};
		Runtime = FItemRuntimeState{};
		InsertedMagazine = nullptr;
		MagazineCurrentAmmo = 0;
		MagazineLoadedAmmoType = EAmmoType::AmmoType_None;
		MagazineAmmoUnitWeight = 0.f;
		return;
	}

	ItemDetails       = SourceAsset->ItemDetails;
	TradeConfig       = SourceAsset->TradeConfig;
	DurabilityConfig  = SourceAsset->DurabilityConfig;
	ChargeConfig      = SourceAsset->ChargeConfig;
	OutfitStatsConfig = SourceAsset->OutfitStatsConfig;
	WeaponStatsConfig = SourceAsset->WeaponStatsConfig;
	ConsumablesStats  = SourceAsset->ConsumablesStats;
	MagazineConfig	  = SourceAsset->MagazineConfig;

	// Reset runtime attachments
	InsertedMagazine = nullptr;
	MagazineCurrentAmmo = 0;
	MagazineLoadedAmmoType = EAmmoType::AmmoType_None;
	MagazineAmmoUnitWeight = 0.f;

	// Runtime init
	Runtime.StackCount = 1;
	SetStackCount(InStackCount);

	Runtime.CurrDurability = DurabilityConfig.bHasDurability ? DurabilityConfig.MaxDurability : 0.f;
	Runtime.CurrCharge     = ChargeConfig.bHasCharge ? ChargeConfig.MaxCharge : 0.f;
	Runtime.bIsRotated     = false;
}

void UItemObject::SetStackCount(int32 NewCount)
{
	NewCount = FMath::Max(1, NewCount);

	if (!ItemDetails.bIsStackable)
	{
		Runtime.StackCount = 1;
		return;
	}

	const int32 MaxStack = FMath::Max(1, ItemDetails.MaxStackCount);
	Runtime.StackCount = FMath::Clamp(NewCount, 1, MaxStack);
}

USoundBase* UItemObject::GetSoundOfUse() const
{
	return ItemDetails.ItemUseSound;
}

void UItemObject::GetDimensions(FItemSize& Dimensions) const
{
	Dimensions = ItemDetails.Size;

	// Учитываем поворот (как инвентарь раскладывает предмет)
	if (Runtime.bIsRotated)
	{
		Swap(Dimensions.X, Dimensions.Y);
	}

	// Safety
	Dimensions.X = FMath::Max(1, Dimensions.X);
	Dimensions.Y = FMath::Max(1, Dimensions.Y);
}

USoundBase* UItemObject::GetEquipmentSound() const
{
	return ItemDetails.ItemEquipSound;
}

USoundBase* UItemObject::GetDropSound() const
{
	return ItemDetails.ItemDropSound;
}

USoundBase* UItemObject::GetPickupSound() const
{
	return ItemDetails.ItemPickupSound;
}

FItemOutfitStatsConfig UItemObject::GetOutfitStats() const
{
	return OutfitStatsConfig;
}

void UItemObject::SetMagazineLoadedAmmoType(EAmmoType NewType)
{
	if (!IsMagazine())
	{
		MagazineLoadedAmmoType = EAmmoType::AmmoType_None;
		return;
	}

	// Allow reset to None only if empty
	if (NewType == EAmmoType::AmmoType_None)
	{
		if (MagazineCurrentAmmo <= 0)
		{
			MagazineLoadedAmmoType = EAmmoType::AmmoType_None;
			MagazineAmmoUnitWeight = 0.f;
		}
		return;
	}

	// If already set and not matching -> do not mix
	if (MagazineLoadedAmmoType != EAmmoType::AmmoType_None && MagazineLoadedAmmoType != NewType)
	{
		return;
	}

	MagazineLoadedAmmoType = NewType;
}

bool UItemObject::IsAmmoCompatibleForMagazine(EAmmoType AmmoType) const
{
	if (!IsMagazine() || AmmoType == EAmmoType::AmmoType_None)
	{
		return false;
	}

	return MagazineConfig.CompatibleAmmoTypes.Contains(AmmoType);
}

bool UItemObject::IsMagazineCompatibleForWeapon(const UItemObject* MagItem) const
{
	if (!IsWeapon() || !IsValid(MagItem) || !MagItem->IsMagazine())
	{
		return false;
	}

	// Determine magazine type
	EMagazineType MagType = MagItem->ItemDetails.MagazineType;
	if (MagType == EMagazineType::Mag_None)
	{
		MagType = MagItem->MagazineConfig.MagazineType;
	}

	// Prefer explicit compatibility list
	if (WeaponStatsConfig.CompatibleMagazines.Num() > 0)
	{
		return WeaponStatsConfig.CompatibleMagazines.Contains(MagType);
	}

	// Fallback: match weapon ammo type to magazine compatible ammo types
	const EAmmoType WeaponAmmo = WeaponStatsConfig.AmmoType;
	if (WeaponAmmo == EAmmoType::AmmoType_None)
	{
		return false;
	}

	return MagItem->IsAmmoCompatibleForMagazine(WeaponAmmo);
}
