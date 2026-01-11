#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/MasterItemEnums.h"
#include "EquipmentComponent.generated.h"

class UItemObject;
class UInventoryComponent;

UENUM(BlueprintType)
enum class EEquipmentSlotId : uint8
{
	None UMETA(DisplayName="None"),

	// Outfit
	HelmetSlot   UMETA(DisplayName="Helmet"),
	ArmorSlot    UMETA(DisplayName="Armor"),
	BackpackSlot UMETA(DisplayName="Backpack"),

	// Weapons
	PrimaryWeaponSlot   UMETA(DisplayName="Primary Weapon"),
	SecondaryWeaponSlot UMETA(DisplayName="Secondary Weapon"),
	PistolSlot          UMETA(DisplayName="Pistol"),
	KnifeSlot           UMETA(DisplayName="Knife"),

	// Devices
	DeviceSlot1 UMETA(DisplayName="Device 1"),
	DeviceSlot2 UMETA(DisplayName="Device 2"),
	DeviceSlot3 UMETA(DisplayName="Device 3"),

	// Grenades / Bolts / Flare
	GrenadePrimarySlot   UMETA(DisplayName="Grenade Primary"),
	GrenadeSecondarySlot UMETA(DisplayName="Grenade Secondary"),

	// Modules / Artefacts
	ItemSlot1 UMETA(DisplayName="Item Slot 1"),
	ItemSlot2 UMETA(DisplayName="Item Slot 2"),
	ItemSlot3 UMETA(DisplayName="Item Slot 3"),
	ItemSlot4 UMETA(DisplayName="Item Slot 4"),
	ItemSlot5 UMETA(DisplayName="Item Slot 5"),

	// Quick access
	QuickSlot1 UMETA(DisplayName="Quick Slot 1"),
	QuickSlot2 UMETA(DisplayName="Quick Slot 2"),
	QuickSlot3 UMETA(DisplayName="Quick Slot 3"),
	QuickSlot4 UMETA(DisplayName="Quick Slot 4"),

	Slot_Count UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FEquipmentSlotState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
	TObjectPtr<UItemObject> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	bool bLocked = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentSlotChanged, EEquipmentSlotId, SlotId, UItemObject*, Item);
// Broadcast when ActiveSlot/PrevSlot changes (hover highlight during drag&drop)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentActiveSlotChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSelectedSlotChanged, EEquipmentSlotId, SlotId);

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class UESTALKER_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ===== Events =====
	UPROPERTY(BlueprintAssignable, Category="Equipment|Events")
	FOnEquipmentSlotChanged OnEquipmentSlotChanged;

	UPROPERTY(BlueprintAssignable, Category="Equipment|Events")
	FOnEquipmentActiveSlotChanged OnEquipmentActiveSlotChanged;

	UPROPERTY(BlueprintAssignable, Category="Equipment|Events")
	FOnEquipmentSelectedSlotChanged OnEquipmentSelectedSlotChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment|Select")
	EEquipmentSlotId SelectedSlot = EEquipmentSlotId::None;

	UFUNCTION(BlueprintCallable, Category="Equipment|Select")
	void SetSelectedSlot(EEquipmentSlotId SlotId);

	// ===== Hover state (Drag&Drop подсветка) =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment|Drag")
	EEquipmentSlotId ActiveSlot = EEquipmentSlotId::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment|Drag")
	EEquipmentSlotId PrevSlot = EEquipmentSlotId::None;

	// ===== API =====
	UFUNCTION(BlueprintCallable, Category="Equipment")
	bool EquipToSlot(EEquipmentSlotId SlotId, UItemObject* Item, bool bTryRemoveFromInventory = true);

	UFUNCTION(BlueprintCallable, Category="Equipment")
	bool UnequipSlot(EEquipmentSlotId SlotId, bool bTryReturnToInventory = true);

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void ActivateSlot(EEquipmentSlotId SlotId, UItemObject* DragItem);

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void ClearActiveSlot();

	UFUNCTION(BlueprintPure, Category="Equipment")
	UItemObject* GetActiveItem() const;

	// ===== Helpers =====
	UFUNCTION(BlueprintPure, Category="Equipment")
	UItemObject* GetItemInSlot(EEquipmentSlotId SlotId) const;

	UFUNCTION(BlueprintPure, Category="Equipment")
	bool IsSlotOccupied(EEquipmentSlotId SlotId) const;

	UFUNCTION(BlueprintPure, Category="Equipment")
	bool IsSlotLocked(EEquipmentSlotId SlotId) const;

	UFUNCTION(BlueprintPure, Category="Equipment")
	bool IsSlotBlocked(EEquipmentSlotId SlotId) const;

	UFUNCTION(BlueprintPure, Category="Equipment")
	bool CanEquipItemToSlot(UItemObject* Item, EEquipmentSlotId SlotId) const;

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void SetSlotLocked(EEquipmentSlotId SlotId, bool bLocked);

	// ===== BlockedSlots logic =====
	// “броня открывает N слотов под артефакты/модули”
	UFUNCTION(BlueprintCallable, Category="Equipment|Blocked")
	void SetArmorModuleSlots(int32 UnlockedSlots, int32 MaxSlots, int32 UnlockedAfterUpgrade = 0);

	UFUNCTION(BlueprintCallable, Category="Equipment|Blocked")
	void RebuildBlockedSlots();

	// ===== References =====
	UFUNCTION(BlueprintPure, Category="Equipment")
	UInventoryComponent* GetInventoryRef() const { return InventoryRef; }

private:
	UPROPERTY()
	TObjectPtr<UInventoryComponent> InventoryRef = nullptr;

	UPROPERTY()
	TArray<FEquipmentSlotState> Slots;

	UPROPERTY()
	TArray<bool> Blocked;

	// Пока это живёт тут (позже перенести в статы Outfits)
	UPROPERTY()
	int32 ArmorModuleSlotsUnlocked = 0;

	UPROPERTY()
	int32 ArmorModuleSlotsMax = 0;

	UPROPERTY()
	int32 ArmorModuleSlotsUnlockedAfterUpgrade = 0;

private:
	int32 ToIndex(EEquipmentSlotId SlotId) const;
	bool IsValidSlot(EEquipmentSlotId SlotId) const;

	EEquipmentSlotId FindSlotByItem(const UItemObject* Item) const;
	bool RemoveFromInventoryIfPresent(UItemObject* Item) const;

	static bool IsPrimarySecondaryWeaponSubCat(EItemSubCategory SubCat);
	void BroadcastChanged(EEquipmentSlotId SlotId);

	static bool HasAnyTag(const TArray<FName>& ItemTags, const TArray<FName>& QueryTags);

	static bool IsAllowedByLists(
		const UItemObject* Candidate,
		const TArray<int32>& AllowedIDs,
		const TArray<int32>& BlockedIDs,
		const TArray<FName>& AllowedTags,
		const TArray<FName>& BlockedTags
	);

	bool IsHelmetCompatibleWithArmor(const UItemObject* Helmet, const UItemObject* Armor) const;
	bool IsBackpackCompatibleWithArmor(const UItemObject* Backpack, const UItemObject* Armor) const;

	// читает OutfitStatsConfig брони и вызывает SetArmorModuleSlots(...)
	void ApplyArmorModuleSlotsFromEquippedArmor();
};
