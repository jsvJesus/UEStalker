#include "Components/EquipmentComponent.h"
#include "Components/InventoryComponent.h"
#include "Items/ItemObject.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	Slots.SetNum(static_cast<int32>(EEquipmentSlotId::Slot_Count));
	Blocked.SetNumZeroed(static_cast<int32>(EEquipmentSlotId::Slot_Count));
}

void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor))
	{
		InventoryRef = OwnerActor->FindComponentByClass<UInventoryComponent>();
	}

	RebuildBlockedSlots();
}

void UEquipmentComponent::SetSelectedSlot(EEquipmentSlotId SlotId)
{
	// None разрешаем
	if (SlotId != EEquipmentSlotId::None && !IsValidSlot(SlotId))
	{
		return;
	}

	if (SelectedSlot == SlotId)
	{
		return;
	}

	SelectedSlot = SlotId;
	OnEquipmentSelectedSlotChanged.Broadcast(SelectedSlot);
}

bool UEquipmentComponent::EquipToSlot(EEquipmentSlotId SlotId, UItemObject* Item, bool bTryRemoveFromInventory)
{
	if (!IsValid(Item) || !IsValidSlot(SlotId))
	{
		return false;
	}

	if (IsSlotLocked(SlotId) || IsSlotBlocked(SlotId))
	{
		return false;
	}

	// занято (без свапа)
	if (IsSlotOccupied(SlotId))
	{
		return false;
	}

	if (!CanEquipItemToSlot(Item, SlotId))
	{
		return false;
	}

	// Если предмет уже был в другом слоте — снимаем оттуда (без возврата в инвентарь)
	const EEquipmentSlotId FromSlot = FindSlotByItem(Item);
	if (FromSlot != EEquipmentSlotId::None)
	{
		Slots[ToIndex(FromSlot)].Item = nullptr;
		BroadcastChanged(FromSlot);
	}

	// Если предмет лежал в инвентаре — убираем из грида
	if (bTryRemoveFromInventory)
	{
		RemoveFromInventoryIfPresent(Item);
	}

	// Если надеваем броню — она может конфликтовать с текущим Helmet/Backpack
	if (SlotId == EEquipmentSlotId::ArmorSlot)
	{
		UItemObject* CurrentHelmet = GetItemInSlot(EEquipmentSlotId::HelmetSlot);
		UItemObject* CurrentBackpack = GetItemInSlot(EEquipmentSlotId::BackpackSlot);

		// если броня запрещает/не разрешает текущий шлем — пытаемся снять в инвентарь
		if (IsValid(CurrentHelmet) && !IsHelmetCompatibleWithArmor(CurrentHelmet, Item))
		{
			if (!UnequipSlot(EEquipmentSlotId::HelmetSlot, true))
			{
				return false; // нет места, не надеваем броню
			}
		}

		// если броня запрещает/не разрешает текущий рюкзак — пытаемся снять в инвентарь
		if (IsValid(CurrentBackpack) && !IsBackpackCompatibleWithArmor(CurrentBackpack, Item))
		{
			if (!UnequipSlot(EEquipmentSlotId::BackpackSlot, true))
			{
				return false;
			}
		}
	}

	// Положили
	Slots[ToIndex(SlotId)].Item = Item;

	// экипировка влияет на общий переносимый вес (кэш находится в InventoryComponent)
	if (IsValid(InventoryRef))
	{
		InventoryRef->InvalidateWeight();
	}

	if (SlotId == EEquipmentSlotId::ArmorSlot)
	{
		ApplyArmorModuleSlotsFromEquippedArmor();
		RebuildBlockedSlots();
	}

	// звук экипировки (если задан)
	if (USoundBase* Snd = Item->GetEquipmentSound())
	{
		AActor* OwnerActor = GetOwner();
		const FVector Loc = IsValid(OwnerActor) ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
		UGameplayStatics::PlaySoundAtLocation(this, Snd, Loc);
	}

	BroadcastChanged(SlotId);

	// броня влияет на блокировки (слоты артефактов/модулей и будущую совместимость)
	if (SlotId == EEquipmentSlotId::ArmorSlot || SlotId == EEquipmentSlotId::HelmetSlot || SlotId == EEquipmentSlotId::BackpackSlot)
	{
		RebuildBlockedSlots();
	}

	return true;
}

bool UEquipmentComponent::UnequipSlot(EEquipmentSlotId SlotId, bool bTryReturnToInventory)
{
	if (!IsValidSlot(SlotId))
	{
		return false;
	}

	UItemObject* Item = Slots[ToIndex(SlotId)].Item;
	if (!IsValid(Item))
	{
		return false;
	}

	// если надо вернуть в инвентарь — пробуем положить
	if (bTryReturnToInventory && IsValid(InventoryRef))
	{
		if (!InventoryRef->TryAddItem(Item))
		{
			return false; // нет места -> НЕ снимаем
		}
	}

	// звук экипировки (снятие тоже)
	if (USoundBase* Snd = Item->GetEquipmentSound())
	{
		AActor* OwnerActor = GetOwner();
		const FVector Loc = IsValid(OwnerActor) ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
		UGameplayStatics::PlaySoundAtLocation(this, Snd, Loc);
	}

	Slots[ToIndex(SlotId)].Item = nullptr;

	// снятие тоже влияет на вес
	if (IsValid(InventoryRef))
	{
		InventoryRef->InvalidateWeight();
	}
	
	if (SlotId == EEquipmentSlotId::ArmorSlot)
	{
		ApplyArmorModuleSlotsFromEquippedArmor(); // это выставит 0,0,0
		RebuildBlockedSlots();
	}
	
	BroadcastChanged(SlotId);

	if (SlotId == EEquipmentSlotId::ArmorSlot || SlotId == EEquipmentSlotId::HelmetSlot || SlotId == EEquipmentSlotId::BackpackSlot)
	{
		RebuildBlockedSlots();
	}

	return true;
}

void UEquipmentComponent::ActivateSlot(EEquipmentSlotId SlotId, UItemObject* DragItem)
{
	ActiveSlot = EEquipmentSlotId::None;
	PrevSlot = EEquipmentSlotId::None;

	if (!IsValidSlot(SlotId))
	{
		return;
	}

	// если слот вообще не принимает (заблокирован/залочен/занят) — красный
	const bool bCanDrop =
		!IsSlotLocked(SlotId) &&
		!IsSlotBlocked(SlotId) &&
		!IsSlotOccupied(SlotId) &&
		(IsValid(DragItem) ? CanEquipItemToSlot(DragItem, SlotId) : true);

	if (bCanDrop)
	{
		ActiveSlot = SlotId;
	}
	else
	{
		PrevSlot = SlotId;
	}

	OnEquipmentActiveSlotChanged.Broadcast();
}

void UEquipmentComponent::ClearActiveSlot()
{
	ActiveSlot = EEquipmentSlotId::None;
	PrevSlot = EEquipmentSlotId::None;
	OnEquipmentActiveSlotChanged.Broadcast();
}

UItemObject* UEquipmentComponent::GetActiveItem() const
{
	return GetItemInSlot(ActiveSlot);
}

bool UEquipmentComponent::IsSlotOccupied(EEquipmentSlotId SlotId) const
{
	return IsValid(GetItemInSlot(SlotId));
}

void UEquipmentComponent::SetArmorModuleSlots(int32 UnlockedSlots, int32 MaxSlots, int32 UnlockedAfterUpgrade)
{
	ArmorModuleSlotsUnlocked = FMath::Clamp(UnlockedSlots, 0, 5);
	ArmorModuleSlotsMax = FMath::Clamp(MaxSlots, 0, 5);
	ArmorModuleSlotsUnlockedAfterUpgrade = FMath::Clamp(UnlockedAfterUpgrade, 0, 5);

	RebuildBlockedSlots();
}

void UEquipmentComponent::RebuildBlockedSlots()
{
	// сброс
	for (int32 i = 0; i < Blocked.Num(); ++i)
	{
		Blocked[i] = false;
	}

	// --- Armor / module slots ---
	UItemObject* Armor = GetItemInSlot(EEquipmentSlotId::ArmorSlot);

	int32 MaxSlots = 0;
	int32 Unlocked = 0;

	if (IsValid(Armor))
	{
		MaxSlots = FMath::Clamp(ArmorModuleSlotsMax, 0, 5);
		Unlocked = FMath::Clamp(ArmorModuleSlotsUnlocked, 0, MaxSlots);
	}
	// если брони нет -> MaxSlots/Unlocked = 0 (все 5 закрыты)

	const EEquipmentSlotId ItemSlots[5] =
	{
		EEquipmentSlotId::ItemSlot1,
		EEquipmentSlotId::ItemSlot2,
		EEquipmentSlotId::ItemSlot3,
		EEquipmentSlotId::ItemSlot4,
		EEquipmentSlotId::ItemSlot5
	};

	for (int32 i = 0; i < 5; ++i)
	{
		const bool bBlocked = (i >= MaxSlots) || (i >= Unlocked);
		Blocked[ToIndex(ItemSlots[i])] = bBlocked;
	}

	// --- Armor blocks external helmet/backpack ---
	if (IsValid(Armor))
	{
		const FItemOutfitStatsConfig& ACfg = Armor->OutfitStatsConfig;

		if (!ACfg.bAllowExternalHelmet)
		{
			Blocked[ToIndex(EEquipmentSlotId::HelmetSlot)] = true;
		}
		if (!ACfg.bAllowExternalBackpack)
		{
			Blocked[ToIndex(EEquipmentSlotId::BackpackSlot)] = true;
		}
	}

	// UI refresh
	for (int32 i = 1; i < static_cast<int32>(EEquipmentSlotId::Slot_Count); ++i)
	{
		BroadcastChanged(static_cast<EEquipmentSlotId>(i));
	}
}

bool UEquipmentComponent::IsSlotLocked(EEquipmentSlotId SlotId) const
{
	if (!IsValidSlot(SlotId))
	{
		return true;
	}
	return Slots[ToIndex(SlotId)].bLocked;
}

bool UEquipmentComponent::IsSlotBlocked(EEquipmentSlotId SlotId) const
{
	if (!IsValidSlot(SlotId))
	{
		return true;
	}
	return Blocked[ToIndex(SlotId)];
}

bool UEquipmentComponent::CanEquipItemToSlot(UItemObject* Item, EEquipmentSlotId SlotId) const
{
	if (!IsValid(Item) || !IsValidSlot(SlotId))
	{
		return false;
	}

	if (IsSlotLocked(SlotId) || IsSlotBlocked(SlotId))
	{
		return false;
	}

	const EItemCategory Cat = Item->ItemDetails.ItemCategory;
	const EItemSubCategory Sub = Item->ItemDetails.ItemSubCategory;

	switch (SlotId)
	{
		// Outfit
		case EEquipmentSlotId::ArmorSlot:
			return Cat == EItemCategory::ItemCat_Armor;

		case EEquipmentSlotId::HelmetSlot:
		{
			if (Cat != EItemCategory::ItemCat_Helmet) return false;

			UItemObject* Armor = GetItemInSlot(EEquipmentSlotId::ArmorSlot);
			if (IsValid(Armor))
			{
				return IsHelmetCompatibleWithArmor(Item, Armor);
			}
			return true;
		}

		case EEquipmentSlotId::BackpackSlot:
		{
			if (Cat != EItemCategory::ItemCat_Backpack) return false;

			UItemObject* Armor = GetItemInSlot(EEquipmentSlotId::ArmorSlot);
			if (IsValid(Armor))
			{
				return IsBackpackCompatibleWithArmor(Item, Armor);
			}
			return true;
		}

		// Weapons
		case EEquipmentSlotId::PrimaryWeaponSlot:
		case EEquipmentSlotId::SecondaryWeaponSlot:
			return (Cat == EItemCategory::ItemCat_Weapons) && IsPrimarySecondaryWeaponSubCat(Sub);

		case EEquipmentSlotId::PistolSlot:
			return Sub == EItemSubCategory::ItemSubCat_Weapons_HG;

		case EEquipmentSlotId::KnifeSlot:
			return Sub == EItemSubCategory::ItemSubCat_Weapons_Knife;

		// Devices
		case EEquipmentSlotId::DeviceSlot1:
		case EEquipmentSlotId::DeviceSlot2:
		case EEquipmentSlotId::DeviceSlot3:
			return Cat == EItemCategory::ItemCat_UsableItems;

		// Grenades
		case EEquipmentSlotId::GrenadePrimarySlot:
		case EEquipmentSlotId::GrenadeSecondarySlot:
			return Sub == EItemSubCategory::ItemSubCat_Weapons_Grenade;

		// Modules / Artefacts
		case EEquipmentSlotId::ItemSlot1:
		case EEquipmentSlotId::ItemSlot2:
		case EEquipmentSlotId::ItemSlot3:
		case EEquipmentSlotId::ItemSlot4:
		case EEquipmentSlotId::ItemSlot5:
			return (Sub == EItemSubCategory::ItemSubCat_Items_Artefacts ||
					Sub == EItemSubCategory::ItemSubCat_Items_Modules);

		// Quick access
		case EEquipmentSlotId::QuickSlot1:
		case EEquipmentSlotId::QuickSlot2:
		case EEquipmentSlotId::QuickSlot3:
		case EEquipmentSlotId::QuickSlot4:
			return (Sub == EItemSubCategory::ItemSubCat_Items_Food ||
					Sub == EItemSubCategory::ItemSubCat_Items_Water ||
					Sub == EItemSubCategory::ItemSubCat_Items_Medicine);

		default:
			break;
	}

	return false;
}

void UEquipmentComponent::SetSlotLocked(EEquipmentSlotId SlotId, bool bLocked)
{
	if (!IsValidSlot(SlotId))
	{
		return;
	}
	Slots[ToIndex(SlotId)].bLocked = bLocked;
	BroadcastChanged(SlotId);
}

UItemObject* UEquipmentComponent::GetItemInSlot(EEquipmentSlotId SlotId) const
{
	if (SlotId == EEquipmentSlotId::None)
	{
		return nullptr;
	}

	if (!IsValidSlot(SlotId))
	{
		return nullptr;
	}

	const int32 Index = ToIndex(SlotId);
	if (!Slots.IsValidIndex(Index))
	{
		return nullptr;
	}

	return Slots[Index].Item;
}

int32 UEquipmentComponent::ToIndex(EEquipmentSlotId SlotId) const
{
	return static_cast<int32>(SlotId);
}

bool UEquipmentComponent::IsValidSlot(EEquipmentSlotId SlotId) const
{
	return SlotId != EEquipmentSlotId::None
		&& ToIndex(SlotId) > 0
		&& ToIndex(SlotId) < static_cast<int32>(EEquipmentSlotId::Slot_Count);
}

EEquipmentSlotId UEquipmentComponent::FindSlotByItem(const UItemObject* Item) const
{
	if (!IsValid(Item))
	{
		return EEquipmentSlotId::None;
	}

	for (int32 i = 1; i < Slots.Num(); ++i)
	{
		if (Slots[i].Item == Item)
		{
			return static_cast<EEquipmentSlotId>(i);
		}
	}
	return EEquipmentSlotId::None;
}

bool UEquipmentComponent::RemoveFromInventoryIfPresent(UItemObject* Item) const
{
	if (!IsValid(Item) || !IsValid(InventoryRef))
	{
		return false;
	}

	// если предмет лежит в гриде — удаляем оттуда перед экипировкой
	if (InventoryRef->Items.Contains(Item))
	{
		InventoryRef->RemoveItem(Item);
		return true;
	}

	return false;
}

bool UEquipmentComponent::IsPrimarySecondaryWeaponSubCat(EItemSubCategory SubCat)
{
	// Primary/Secondary берут всё оружие кроме: HG, Knife, Grenade
	if (SubCat == EItemSubCategory::ItemSubCat_Weapons_HG) return false;
	if (SubCat == EItemSubCategory::ItemSubCat_Weapons_Knife) return false;
	if (SubCat == EItemSubCategory::ItemSubCat_Weapons_Grenade) return false;

	// Это “оружейная” ветка: ASR/SNP/SMG/SHTG/MG/RPG и т.п.
	return (SubCat >= EItemSubCategory::ItemSubCat_Weapons_ASR &&
			SubCat <= EItemSubCategory::ItemSubCat_Weapons_Grenade)
			&& SubCat != EItemSubCategory::ItemSubCat_Weapons_Grenade;
}

void UEquipmentComponent::BroadcastChanged(EEquipmentSlotId SlotId)
{
	OnEquipmentSlotChanged.Broadcast(SlotId, GetItemInSlot(SlotId));
}

bool UEquipmentComponent::HasAnyTag(const TArray<FName>& ItemTags, const TArray<FName>& QueryTags)
{
	if (ItemTags.Num() == 0 || QueryTags.Num() == 0) return false;

	for (const FName& Q : QueryTags)
	{
		if (ItemTags.Contains(Q))
		{
			return true;
		}
	}
	return false;
}

bool UEquipmentComponent::IsAllowedByLists(const UItemObject* Candidate, const TArray<int32>& AllowedIDs,
	const TArray<int32>& BlockedIDs, const TArray<FName>& AllowedTags, const TArray<FName>& BlockedTags)
{
	if (!IsValid(Candidate)) return false;

	const int32 Id = Candidate->ItemDetails.ItemID;
	const TArray<FName>& Tags = Candidate->ItemDetails.ItemTags;

	// hard-block
	if (BlockedIDs.Contains(Id))
	{
		return false;
	}
	if (BlockedTags.Num() > 0 && HasAnyTag(Tags, BlockedTags))
	{
		return false;
	}

	// allow-list (если задана — нужно пройти)
	if (AllowedIDs.Num() > 0 && !AllowedIDs.Contains(Id))
	{
		return false;
	}
	if (AllowedTags.Num() > 0 && !HasAnyTag(Tags, AllowedTags))
	{
		return false;
	}

	return true;
}

bool UEquipmentComponent::IsHelmetCompatibleWithArmor(const UItemObject* Helmet, const UItemObject* Armor) const
{
	if (!IsValid(Helmet) || !IsValid(Armor)) return true;

	const FItemOutfitStatsConfig& ACfg = Armor->OutfitStatsConfig;

	if (!ACfg.bAllowExternalHelmet)
	{
		return false;
	}

	return IsAllowedByLists(
		Helmet,
		ACfg.AllowedHelmetItemIDs,
		ACfg.BlockedHelmetItemIDs,
		ACfg.AllowedHelmetTags,
		ACfg.BlockedHelmetTags);
}

bool UEquipmentComponent::IsBackpackCompatibleWithArmor(const UItemObject* Backpack, const UItemObject* Armor) const
{
	if (!IsValid(Backpack) || !IsValid(Armor)) return true;

	const FItemOutfitStatsConfig& ACfg = Armor->OutfitStatsConfig;

	if (!ACfg.bAllowExternalBackpack)
	{
		return false;
	}

	return IsAllowedByLists(
		Backpack,
		ACfg.AllowedBackpackItemIDs,
		ACfg.BlockedBackpackItemIDs,
		ACfg.AllowedBackpackTags,
		ACfg.BlockedBackpackTags);
}

void UEquipmentComponent::ApplyArmorModuleSlotsFromEquippedArmor()
{
	UItemObject* Armor = GetItemInSlot(EEquipmentSlotId::ArmorSlot);
	if (!IsValid(Armor))
	{
		SetArmorModuleSlots(0, 0, 0);
		return;
	}

	const FItemOutfitStatsConfig& ACfg = Armor->OutfitStatsConfig;

	// clamp на всякий
	const int32 MaxSlots = FMath::Clamp(ACfg.MaxModuleSlots, 0, 5);
	const int32 Unlocked = FMath::Clamp(ACfg.UnlockedModuleSlots, 0, MaxSlots);
	const int32 AfterUpg = FMath::Clamp(ACfg.UnlockedAfterUpgrade, 0, MaxSlots);

	SetArmorModuleSlots(Unlocked, MaxSlots, AfterUpg);
}
