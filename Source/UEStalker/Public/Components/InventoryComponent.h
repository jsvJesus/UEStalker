#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/MasterItemStructs.h"
#include "InventoryComponent.generated.h"

class UItemObject;
class AMasterItemActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUsed, UItemObject*, Item);

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class UESTALKER_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// Кол-во столбцов в инвентаре
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(ClampMin="1", UIMin="1"))
	int32 Columns;

	// Кол-во строк в инвентаре
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(ClampMin="1", UIMin="1"))
	int32 Rows;

	// Слоты инвентаря (1D массив: Index = Y * Columns + X)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TArray<TObjectPtr<UItemObject>> Items;

	/** 0 = без лимита */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory|Weight", meta=(ClampMin="0.0", UIMin="0.0"))
	float MaxCarryWeight = 50.f;

	// ==== Events ====
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnItemUsed OnItemUsed;

	// ==== Flags ====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	bool bIsInventoryChanged = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory|Events")
	bool bIsItemUsed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory|Events")
	TObjectPtr<UItemObject> ItemUsed = nullptr;

	UFUNCTION(BlueprintCallable, Category="Inventory|Events")
	void MarkInventoryChanged();

	UFUNCTION(BlueprintCallable, Category="Inventory|Events")
	void MarkItemUsed(UItemObject* Item);

	UFUNCTION(BlueprintPure, Category="Inventory")
	FORCEINLINE int32 GetCapacity() const { return Columns * Rows; }

	// =========================================
	// Weight cache API
	// =========================================

	UFUNCTION(BlueprintCallable, Category="Inventory|Weight")
	void InvalidateWeight();

	UFUNCTION(BlueprintPure, Category="Inventory|Weight")
	float GetTotalWeight() const;

	UFUNCTION(BlueprintPure, Category="Inventory|Weight")
	FORCEINLINE float GetMaxCarryWeight() const { return MaxCarryWeight; }

	/** Проверка перегруза по предмету (учитывает StackCount * ItemWeight) */
	UFUNCTION(BlueprintPure, Category="Inventory|Weight")
	bool CanTakeItem(const UItemObject* ItemObject) const;

	/** Проверка перегруза по добавочному весу */
	UFUNCTION(BlueprintPure, Category="Inventory|Weight")
	bool CanTakeAdditionalWeight(float AddWeight) const;

	// =========================================
	// Requested functions
	// =========================================
	
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SetGridSize(int32 NewColumns, int32 NewRows);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void GetItemAtIndex(int32 Index, bool& bValid, UItemObject*& ItemObject) const;

	/** Вернуть массив уникальных предметов (без nullptr и без дублей). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Inventory")
	TArray<UItemObject*> GetAllItems() const;

	/** Index -> Tile (X,Y). Если Index вне диапазона - вернет X=-1,Y=-1 */
	UFUNCTION(BlueprintPure, Category="Inventory")
	FTile IndexToTile(int32 Index) const;

	/** Tile (X,Y) -> Index. Если Tile вне сетки - вернет INDEX_NONE */
	UFUNCTION(BlueprintPure, Category="Inventory")
	int32 TileToIndex(const FTile& Tile) const;

	/** Записать ItemObject во все клетки, начиная с TopLeftIndex (учитывает поворот Runtime.bIsRotated). */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void AddItemAt(UItemObject* ItemObject, int32 TopLeftIndex);

	/** Проверка: помещается ли предмет (с учетом размера/поворота) начиная с TopLeftIndex. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Inventory")
	bool IsRoomAvailable(UItemObject* ItemObject, int32 TopLeftIndex) const;

	/** Попытаться положить предмет в первый подходящий слот (TopLeft). */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool TryAddItem(UItemObject* ItemObject);

	/** Удалить предмет из всех ячеек (очистить ссылки). */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void RemoveItem(UItemObject* ItemObject);

	/** Дропнуть предмет в мир (спавн по ItemClass) и удалить из инвентаря. */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void DropItem(AActor* Actor, UItemObject* ItemObject, bool bGroundClamp = true);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void UseItem(UItemObject* ItemObject);

	// =========================================
	// Weapon/Magazine/Ammo operations (Drag&Drop)
	// =========================================

	/** Магазин -> Оружие. Удаляет магазин из инвентаря и кладёт его внутрь WeaponItem. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Weapon")
	bool TryAttachMagazineToWeapon(UItemObject* WeaponItem, UItemObject* MagazineItem, bool bAllowSwap = true);

	/** Снять магазин с оружия. Возвращает в инвентарь (если есть место) или отменяет операцию. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Weapon")
	bool TryDetachMagazineFromWeapon(UItemObject* WeaponItem, bool bTryReturnToInventory = true);

	/** Патроны -> Магазин. Загружает максимум (или RequestedCount). Учитывает stack у патронов. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Weapon")
	bool TryLoadAmmoToMagazine(UItemObject* MagazineItem, UItemObject* AmmoItem, int32 RequestedCount, int32& OutLoadedCount);

	/** Универсально для UMG: Payload кинул на Target. Возвращает true если применилось (mag->weapon, ammo->mag, ammo->weapon(через вставленный mag)). */
	UFUNCTION(BlueprintCallable, Category="Inventory|Weapon")
	bool TryApplyItemToItem(UItemObject* Payload, UItemObject* Target, int32 RequestedCount, int32& OutAppliedCount);

	UFUNCTION(BlueprintPure, Category="Inventory|Weapon")
	bool CanAttachMagazineToWeapon(const UItemObject* WeaponItem, const UItemObject* MagazineItem) const;

	UFUNCTION(BlueprintPure, Category="Inventory|Weapon")
	bool CanLoadAmmoToMagazine(const UItemObject* MagazineItem, const UItemObject* AmmoItem) const;

	UFUNCTION(BlueprintPure, Category="Inventory|Grid", meta=(DisplayName="Tile Valid"))
	bool TileValid(int32 TileX, int32 TileY) const;

	UFUNCTION(BlueprintPure, Category="Inventory|Grid", meta=(DisplayName="For Each Index"))
	void ForEachIndex(UItemObject* ItemObject, int32 TopLeftIndex, TArray<FTile>& OutTiles) const;

protected:
	virtual void BeginPlay() override;
	virtual void OnRegister() override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;
	
private:
	FORCEINLINE bool IsTileInBounds(const FTile& Tile) const
	{
		return Tile.X >= 0 && Tile.Y >= 0 && Tile.X < Columns && Tile.Y < Rows;
	}

	FORCEINLINE FIntPoint GetEffectiveItemSize(const UItemObject* ItemObject) const;

	float GetStackWeight(const UItemObject* ItemObject) const;
	bool AreStackCompatible(const UItemObject* A, const UItemObject* B) const;
	
	// Weight cache
	mutable bool bWeightDirty = true;
	mutable float CachedTotalWeight = 0.f;
};