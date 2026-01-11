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
		ConsumablesStats = FConsumablesStats{};
		MagazineConfig = FItemMagazineConfig{};
		Runtime = FItemRuntimeState{};

		InsertedMagazine = nullptr;
		OwnerWeapon = nullptr;
		MagazineAmmoCount = 0;
		MagazineLoadedAmmoType = EAmmoType::AmmoType_None;
		MagazineLoadedAmmoAsset = nullptr;
		
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

	// Runtime init
	Runtime.StackCount = 1;
	SetStackCount(InStackCount);

	Runtime.CurrDurability = DurabilityConfig.bHasDurability ? DurabilityConfig.MaxDurability : 0.f;
	Runtime.CurrCharge     = ChargeConfig.bHasCharge ? ChargeConfig.MaxCharge : 0.f;
	Runtime.bIsRotated     = false;

	// Runtime init: attachments/ammo
	InsertedMagazine = nullptr;
	OwnerWeapon = nullptr;
	MagazineAmmoCount = 0;
	MagazineLoadedAmmoType = EAmmoType::AmmoType_None;
	MagazineLoadedAmmoAsset = nullptr;
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

bool UItemObject::IsWeapon() const
{
	return ItemDetails.ItemCategory == EItemCategory::ItemCat_Weapons;
}

bool UItemObject::IsMagazine() const
{
	// В проекте магазины живут в Attachments->Magazine
	return (ItemDetails.ItemCategory == EItemCategory::ItemCat_Attachments)
		&& (ItemDetails.ItemSubCategory == EItemSubCategory::ItemSubCat_Attachments_Magazine)
		&& (ItemDetails.MagazineType != EMagazineType::Mag_None);
}

bool UItemObject::IsAmmo() const
{
	return (ItemDetails.ItemCategory == EItemCategory::ItemCat_Ammo)
		&& (ItemDetails.AmmoType != EAmmoType::AmmoType_None);
}

int32 UItemObject::GetMagazineCapacity() const
{
	return FMath::Max(0, MagazineConfig.Capacity);
}

FText UItemObject::GetMagazineAmmoText() const
{
	const int32 Cap = GetMagazineCapacity();
	const int32 Curr = FMath::Clamp(MagazineAmmoCount, 0, Cap > 0 ? Cap : INT32_MAX);

	if (Cap > 0)
	{
		return FText::FromString(FString::Printf(TEXT("%d/%d"), Curr, Cap));
	}

	// если Capacity не настроен
	return FText::FromString(FString::Printf(TEXT("%d"), Curr));
}

FText UItemObject::GetAmmoCountText() const
{
	const int32 Count = FMath::Max(1, Runtime.StackCount);
	return FText::AsNumber(Count);
}
