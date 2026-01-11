#include "Components/InventoryComponent.h"
#include "Components/EquipmentComponent.h"
#include "Items/ItemObject.h"
#include "Items/MasterItemActor.h"
#include "Items/MasterItemDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	
	Columns = 8;
	Rows = 11;

	Items.SetNumZeroed(GetCapacity());
	InvalidateWeight();
}

void UInventoryComponent::MarkInventoryChanged()
{
	bIsInventoryChanged = true;
	InvalidateWeight();
}

void UInventoryComponent::MarkItemUsed(UItemObject* Item)
{
	ItemUsed = Item;
	bIsItemUsed = true;

	// Обычно UseItem меняет стак/удаляет предмет -> пусть UI обновится
	MarkInventoryChanged();
}

void UInventoryComponent::InvalidateWeight()
{
	bWeightDirty = true;
}

float UInventoryComponent::GetTotalWeight() const
{
	if (!bWeightDirty)
	{
		return CachedTotalWeight;
	}

	float Total = 0.f;

	TSet<const UItemObject*> Seen;
	Seen.Reserve(Items.Num());

	auto Accumulate = [&](const UItemObject* Obj, auto&& AccumulateRef) -> void
	{
		if (!IsValid(Obj) || Seen.Contains(Obj))
		{
			return;
		}
		Seen.Add(Obj);
		Total += GetStackWeight(Obj);

		// Оружие может содержать магазин (магазин НЕ лежит в инвентаре как отдельный слот)
		if (IsValid(Obj->InsertedMagazine))
		{
			AccumulateRef(Obj->InsertedMagazine, AccumulateRef);
		}
	};

	for (const TObjectPtr<UItemObject>& Slot : Items)
	{
		Accumulate(Slot.Get(), Accumulate);
	}

	// + Equipment weight (equipped items are still carried)
	if (const AActor* OwnerActor = GetOwner())
	{
		if (const UEquipmentComponent* EquipComp = OwnerActor->FindComponentByClass<UEquipmentComponent>())
		{
			const int32 SlotCount = static_cast<int32>(EEquipmentSlotId::Slot_Count);
			for (int32 i = 1; i < SlotCount; ++i) // skip None
			{
				const UItemObject* Eq = EquipComp->GetItemInSlot(static_cast<EEquipmentSlotId>(i));
				Accumulate(Eq, Accumulate);
			}
		}
	}

	CachedTotalWeight = Total;
	bWeightDirty = false;
	return CachedTotalWeight;
}

bool UInventoryComponent::CanTakeItem(const UItemObject* ItemObject) const
{
	if (!IsValid(ItemObject))
	{
		return false;
	}

	// учитываем вложения (магазин в оружии + патроны в магазине)
	float AddWeight = 0.f;
	TSet<const UItemObject*> Seen;
	Seen.Reserve(4);

	auto Accumulate = [&](const UItemObject* Obj, auto&& AccumulateRef) -> void
	{
		if (!IsValid(Obj) || Seen.Contains(Obj))
		{
			return;
		}
		Seen.Add(Obj);
		AddWeight += GetStackWeight(Obj);

		if (IsValid(Obj->InsertedMagazine))
		{
			AccumulateRef(Obj->InsertedMagazine, AccumulateRef);
		}
	};

	Accumulate(ItemObject, Accumulate);
	return CanTakeAdditionalWeight(AddWeight);
}

bool UInventoryComponent::CanTakeAdditionalWeight(float AddWeight) const
{
	if (MaxCarryWeight <= 0.f)
	{
		return true; // без лимита
	}

	AddWeight = FMath::Max(0.f, AddWeight);
	return (GetTotalWeight() + AddWeight) <= (MaxCarryWeight + KINDA_SMALL_NUMBER);
}

void UInventoryComponent::SetGridSize(int32 NewColumns, int32 NewRows)
{
	Columns = FMath::Max(1, NewColumns);
	Rows    = FMath::Max(1, NewRows);

	Items.SetNumZeroed(GetCapacity());
	MarkInventoryChanged();
}

void UInventoryComponent::GetItemAtIndex(int32 Index, bool& bValid, UItemObject*& ItemObject) const
{
	bValid = Items.IsValidIndex(Index);
	ItemObject = bValid ? Items[Index].Get() : nullptr;
}

TArray<UItemObject*> UInventoryComponent::GetAllItems() const
{
	TArray<UItemObject*> AllItems;
	AllItems.Reserve(Items.Num());

	for (const TObjectPtr<UItemObject>& It : Items)
	{
		UItemObject* Obj = It.Get();
		if (IsValid(Obj) && !AllItems.Contains(Obj))
		{
			AllItems.Add(Obj);
		}
	}

	return AllItems;
}

FTile UInventoryComponent::IndexToTile(int32 Index) const
{
	FTile Out;
	Out.X = -1;
	Out.Y = -1;

	const int32 Cap = GetCapacity();
	if (Columns <= 0 || Index < 0 || Index >= Cap)
	{
		return Out;
	}

	Out.X = Index % Columns;
	Out.Y = Index / Columns;
	return Out;
}

int32 UInventoryComponent::TileToIndex(const FTile& Tile) const
{
	if (Columns <= 0 || Rows <= 0)
	{
		return INDEX_NONE;
	}

	if (!IsTileInBounds(Tile))
	{
		return INDEX_NONE;
	}

	return Tile.X + (Tile.Y * Columns);
}

void UInventoryComponent::AddItemAt(UItemObject* ItemObject, int32 TopLeftIndex)
{
	if (!IsValid(ItemObject))
	{
		return;
	}

	// Страховка размера массива
	const int32 Cap = GetCapacity();
	if (Items.Num() != Cap)
	{
		Items.SetNumZeroed(Cap);
	}

	const FTile TopLeftTile = IndexToTile(TopLeftIndex);
	if (!IsTileInBounds(TopLeftTile))
	{
		return;
	}

	const FIntPoint Size = GetEffectiveItemSize(ItemObject);

	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			FTile Tile;
			Tile.X = TopLeftTile.X + X;
			Tile.Y = TopLeftTile.Y + Y;

			const int32 SlotIndex = TileToIndex(Tile);
			if (SlotIndex != INDEX_NONE && Items.IsValidIndex(SlotIndex))
			{
				Items[SlotIndex] = ItemObject;
			}
		}
	}

	MarkInventoryChanged();
}

bool UInventoryComponent::IsRoomAvailable(UItemObject* ItemObject, int32 TopLeftIndex) const
{
	if (!IsValid(ItemObject))
	{
		return false;
	}

	const int32 Cap = GetCapacity();
	if (Cap <= 0 || Items.Num() != Cap)
	{
		return false;
	}

	const FTile TopLeftTile = IndexToTile(TopLeftIndex);
	if (!IsTileInBounds(TopLeftTile))
	{
		return false;
	}

	const FIntPoint Size = GetEffectiveItemSize(ItemObject);

	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			FTile Tile;
			Tile.X = TopLeftTile.X + X;
			Tile.Y = TopLeftTile.Y + Y;

			if (!IsTileInBounds(Tile))
			{
				return false;
			}

			const int32 SlotIndex = TileToIndex(Tile);
			if (!Items.IsValidIndex(SlotIndex))
			{
				return false;
			}

			// Ячейка занята любым предметом -> места нет
			if (IsValid(Items[SlotIndex].Get()))
			{
				return false;
			}
		}
	}

	return true;
}

bool UInventoryComponent::TryAddItem(UItemObject* ItemObject)
{
	if (!IsValid(ItemObject))
	{
		return false;
	}

	// Перегруз
	if (!CanTakeItem(ItemObject))
	{
		return false;
	}

	const int32 Cap = GetCapacity();
	if (Cap <= 0)
	{
		return false;
	}

	if (Items.Num() != Cap)
	{
		Items.SetNumZeroed(Cap);
		InvalidateWeight();
	}

	// если стакуемое — сначала докидываем в существующие стаки
	if (ItemObject->IsStackable())
	{
		int32 Remaining = FMath::Max(1, ItemObject->Runtime.StackCount);
		const int32 MaxStack = FMath::Max(1, ItemObject->GetMaxStack());

		bool bMergedAnything = false;

		if (MaxStack > 1)
		{
			TSet<UItemObject*> Seen;
			Seen.Reserve(Items.Num());

			for (const TObjectPtr<UItemObject>& Slot : Items)
			{
				UItemObject* Existing = Slot.Get();
				if (!IsValid(Existing) || Seen.Contains(Existing))
				{
					continue;
				}
				Seen.Add(Existing);

				if (!AreStackCompatible(Existing, ItemObject))
				{
					continue;
				}

				const int32 Curr = FMath::Max(1, Existing->Runtime.StackCount);
				const int32 Space = FMath::Max(0, MaxStack - Curr);
				if (Space <= 0)
				{
					continue;
				}

				const int32 Add = FMath::Min(Space, Remaining);
				if (Add > 0)
				{
					Existing->SetStackCount(Curr + Add);
					Remaining -= Add;
					bMergedAnything = true;

					if (Remaining <= 0)
					{
						break;
					}
				}
			}
		}

		if (bMergedAnything)
		{
			MarkInventoryChanged(); // обновим UI + вес
		}

		// Полностью докинули в стаки — новый слот не нужен
		if (Remaining <= 0)
		{
			return true;
		}

		// Остаток оставляем в этом же объекте и пробуем положить в грид
		ItemObject->SetStackCount(Remaining);
	}

	// Пытаемся положить предмет в первый подходящий слот
	for (int32 TopLeftIndex = 0; TopLeftIndex < Items.Num(); ++TopLeftIndex)
	{
		if (IsRoomAvailable(ItemObject, TopLeftIndex))
		{
			AddItemAt(ItemObject, TopLeftIndex); // внутри пометим InventoryChanged
			return true;
		}
	}

	return false;
}

void UInventoryComponent::RemoveItem(UItemObject* ItemObject)
{
	if (!IsValid(ItemObject))
	{
		return;
	}

	bool bChanged = false;

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].Get() == ItemObject)
		{
			Items[i] = nullptr;
			bChanged = true;
		}
	}

	if (bChanged)
	{
		MarkInventoryChanged();
	}
}

void UInventoryComponent::DropItem(AActor* Actor, UItemObject* ItemObject, bool bGroundClamp)
{
	if (!IsValid(Actor) || !IsValid(ItemObject))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// SpawnLocation = ActorLocation + Forward * 200
	FVector SpawnLocation = Actor->GetActorLocation() + (Actor->GetActorForwardVector() * 200.f);

	// Ground clamp (LineTrace вниз)
	if (bGroundClamp)
	{
		const FVector TraceStart = SpawnLocation;
		const FVector TraceEnd   = SpawnLocation + FVector(0.f, 0.f, -10000.f);

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(InventoryDropItem), false);
		Params.AddIgnoredActor(Actor);

		const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);
		if (bHit)
		{
			SpawnLocation = Hit.Location;
		}
	}

	TSubclassOf<AActor> ClassToSpawn = ItemObject->ItemDetails.ItemClass;
	if (!ClassToSpawn)
	{
		return;
	}

	// В мире должен лежать pickup-актор (AMasterItemActor).
	// Если ItemClass указывает на "оружие в руках" (или любой актор не-pickup),
	// делаем fallback на AMasterItemActor.
	// Исключение: Placeable — там можно спавнить реальный актор из ItemClass.
	const bool bAllowCustomWorldActor = (ItemObject->ItemDetails.ItemCategory == EItemCategory::ItemCat_Placeable);

	if (!bAllowCustomWorldActor && !ClassToSpawn->IsChildOf(AMasterItemActor::StaticClass()))
	{
		ClassToSpawn = AMasterItemActor::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Actor;
	SpawnParams.Instigator = Actor->GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* Spawned = World->SpawnActor<AActor>(ClassToSpawn, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (!IsValid(Spawned))
	{
		return;
	}

	// Если это AMasterItemActor (или наследник) — заполним данные
	if (AMasterItemActor* Master = Cast<AMasterItemActor>(Spawned))
	{
		Master->ItemData = ItemObject->SourceAsset;
		Master->WorldStackCount = FMath::Max(1, ItemObject->Runtime.StackCount);
	}

	// Если у BP-класса есть переменная ItemObject (ExposeOnSpawn в BP) — установим ее рефлексией
	if (FProperty* Prop = Spawned->GetClass()->FindPropertyByName(TEXT("ItemObject")))
	{
		if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
		{
			if (ObjProp->PropertyClass && ObjProp->PropertyClass->IsChildOf(UItemObject::StaticClass()))
			{
				ObjProp->SetObjectPropertyValue_InContainer(Spawned, ItemObject);
			}
		}
	}

	// Звук дропа (если задан)
	if (ItemObject->ItemDetails.ItemDropSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, ItemObject->ItemDetails.ItemDropSound, SpawnLocation);
	}

	// Удаляем из инвентаря
	RemoveItem(ItemObject);
}

void UInventoryComponent::UseItem(UItemObject* ItemObject)
{
	if (!IsValid(ItemObject))
	{
		return;
	}

	// Уменьшаем стак или удаляем
	const int32 CurrentCount = FMath::Max(1, ItemObject->Runtime.StackCount);

	if (CurrentCount > 1)
	{
		ItemObject->SetStackCount(CurrentCount - 1);
	}
	else
	{
		RemoveItem(ItemObject);
	}

	// Выставляем флаги
	MarkItemUsed(ItemObject);

	// Звук использования
	if (USoundBase* UseSound = ItemObject->GetSoundOfUse())
	{
		const FVector Loc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
		UGameplayStatics::PlaySoundAtLocation(this, UseSound, Loc);
	}
}

bool UInventoryComponent::TryAttachMagazineToWeapon(UItemObject* WeaponItem, UItemObject* MagazineItem, bool bAllowSwap)
{
	if (!CanAttachMagazineToWeapon(WeaponItem, MagazineItem))
	{
		return false;
	}

	// Если уже стоит магазин
	if (IsValid(WeaponItem->InsertedMagazine))
	{
		if (WeaponItem->InsertedMagazine == MagazineItem)
		{
			return true;
		}

		if (!bAllowSwap)
		{
			return false;
		}

		// Пытаемся снять текущий магазин и вернуть в инвентарь
		if (!TryDetachMagazineFromWeapon(WeaponItem, true))
		{
			return false;
		}
	}

	// Убираем магазин из инвентаря (если он там лежал)
	RemoveItem(MagazineItem);

	WeaponItem->InsertedMagazine = MagazineItem;
	MagazineItem->OwnerWeapon = WeaponItem;

	MarkInventoryChanged();
	return true;
}

bool UInventoryComponent::TryDetachMagazineFromWeapon(UItemObject* WeaponItem, bool bTryReturnToInventory)
{
	if (!IsValid(WeaponItem) || !WeaponItem->IsWeapon())
	{
		return false;
	}

	UItemObject* Mag = WeaponItem->InsertedMagazine;
	if (!IsValid(Mag))
	{
		return false;
	}

	// Снимаем
	WeaponItem->InsertedMagazine = nullptr;
	Mag->OwnerWeapon = nullptr;

	if (bTryReturnToInventory)
	{
		// Если не можем вернуть — откатываем
		if (!TryAddItem(Mag))
		{
			WeaponItem->InsertedMagazine = Mag;
			Mag->OwnerWeapon = WeaponItem;
			return false;
		}
	}

	MarkInventoryChanged();
	return true;
}

bool UInventoryComponent::TryLoadAmmoToMagazine(UItemObject* MagazineItem, UItemObject* AmmoItem, int32 RequestedCount,
	int32& OutLoadedCount)
{
	OutLoadedCount = 0;

	if (!CanLoadAmmoToMagazine(MagazineItem, AmmoItem))
	{
		return false;
	}

	const int32 Cap = MagazineItem->GetMagazineCapacity();
	const int32 Curr = FMath::Clamp(MagazineItem->MagazineAmmoCount, 0, Cap);
	const int32 Free = FMath::Max(0, Cap - Curr);
	if (Free <= 0)
	{
		return false;
	}

	const int32 Available = FMath::Max(1, AmmoItem->Runtime.StackCount);
	int32 Want = Free;
	if (RequestedCount > 0)
	{
		Want = FMath::Clamp(RequestedCount, 1, Free);
	}

	const int32 ToLoad = FMath::Min(Want, Available);
	if (ToLoad <= 0)
	{
		return false;
	}

	// если магазин был пуст — фиксируем тип и ассет
	if (Curr == 0)
	{
		MagazineItem->MagazineLoadedAmmoType = AmmoItem->ItemDetails.AmmoType;
		MagazineItem->MagazineLoadedAmmoAsset = AmmoItem->SourceAsset;
	}

	MagazineItem->MagazineAmmoCount = Curr + ToLoad;
	OutLoadedCount = ToLoad;

	// уменьшаем стак патронов / удаляем
	if (Available <= ToLoad)
	{
		RemoveItem(AmmoItem);
		MarkInventoryChanged();
	}
	else
	{
		AmmoItem->SetStackCount(Available - ToLoad);
		MarkInventoryChanged();
	}

	return true;
}

bool UInventoryComponent::TryApplyItemToItem(UItemObject* Payload, UItemObject* Target, int32 RequestedCount,
	int32& OutAppliedCount)
{
	OutAppliedCount = 0;

	if (!IsValid(Payload) || !IsValid(Target) || Payload == Target)
	{
		return false;
	}

	// Mag -> Weapon
	if (Payload->IsMagazine() && Target->IsWeapon())
	{
		return TryAttachMagazineToWeapon(Target, Payload, true);
	}

	// Ammo -> Mag
	if (Payload->IsAmmo() && Target->IsMagazine())
	{
		return TryLoadAmmoToMagazine(Target, Payload, RequestedCount, OutAppliedCount);
	}

	// Ammo -> Weapon (через вставленный Mag)
	if (Payload->IsAmmo() && Target->IsWeapon() && IsValid(Target->InsertedMagazine))
	{
		return TryLoadAmmoToMagazine(Target->InsertedMagazine, Payload, RequestedCount, OutAppliedCount);
	}

	return false;
}

bool UInventoryComponent::CanAttachMagazineToWeapon(const UItemObject* WeaponItem,
                                                    const UItemObject* MagazineItem) const
{
	if (!IsValid(WeaponItem) || !IsValid(MagazineItem))
	{
		return false;
	}

	if (!WeaponItem->IsWeapon() || !MagazineItem->IsMagazine())
	{
		return false;
	}

	// магазин уже где-то стоит
	if (IsValid(MagazineItem->OwnerWeapon))
	{
		return false;
	}

	const EMagazineType MagType = MagazineItem->ItemDetails.MagazineType;
	if (MagType == EMagazineType::Mag_None)
	{
		return false;
	}

	// Основной путь: WeaponStatsConfig.CompatibleMagazines
	if (WeaponItem->WeaponStatsConfig.CompatibleMagazines.Num() > 0)
	{
		return WeaponItem->WeaponStatsConfig.CompatibleMagazines.Contains(MagType);
	}

	// Fallback: по AmmoType (если у оружия не заполнен список совместимых магазинов)
	const EAmmoType WeaponAmmoType = WeaponItem->WeaponStatsConfig.AmmoType;
	if (WeaponAmmoType == EAmmoType::AmmoType_None)
	{
		return false;
	}

	return MagazineItem->MagazineConfig.CompatibleAmmoTypes.Contains(WeaponAmmoType);
}

bool UInventoryComponent::CanLoadAmmoToMagazine(const UItemObject* MagazineItem, const UItemObject* AmmoItem) const
{
	if (!IsValid(MagazineItem) || !IsValid(AmmoItem))
	{
		return false;
	}

	if (!MagazineItem->IsMagazine() || !AmmoItem->IsAmmo())
	{
		return false;
	}

	const int32 Cap = MagazineItem->GetMagazineCapacity();
	if (Cap <= 0)
	{
		return false;
	}

	const int32 Curr = FMath::Max(0, MagazineItem->MagazineAmmoCount);
	if (Curr >= Cap)
	{
		return false; // полный
	}

	const EAmmoType AmmoType = AmmoItem->ItemDetails.AmmoType;
	if (AmmoType == EAmmoType::AmmoType_None)
	{
		return false;
	}

	// если магазин не пуст, можно догружать только тем же типом
	if (Curr > 0 && MagazineItem->MagazineLoadedAmmoType != EAmmoType::AmmoType_None
		&& MagazineItem->MagazineLoadedAmmoType != AmmoType)
	{
		return false;
	}

	// Совместимость по списку
	if (MagazineItem->MagazineConfig.CompatibleAmmoTypes.Num() > 0)
	{
		return MagazineItem->MagazineConfig.CompatibleAmmoTypes.Contains(AmmoType);
	}

	// Fallback: если список пуст — разрешаем любой ammo
	return true;
}

bool UInventoryComponent::TileValid(int32 TileX, int32 TileY) const
{
	// Macros: X>=0, Y>=0, X<Columns, Y<Rows
	return (TileX >= 0) && (TileY >= 0) && (TileX < Columns) && (TileY < Rows);
}

void UInventoryComponent::ForEachIndex(UItemObject* ItemObject, int32 TopLeftIndex, TArray<FTile>& OutTiles) const
{
	OutTiles.Reset();

	if (!IsValid(ItemObject))
	{
		return;
	}

	const FTile TopLeftTile = IndexToTile(TopLeftIndex);
	if (!TileValid(TopLeftTile.X, TopLeftTile.Y))
	{
		return;
	}

	FItemSize Dim;
	ItemObject->GetDimensions(Dim);

	// Macros: внешний цикл X, внутренний Y
	for (int32 X = TopLeftTile.X; X <= TopLeftTile.X + (Dim.X - 1); ++X)
	{
		for (int32 Y = TopLeftTile.Y; Y <= TopLeftTile.Y + (Dim.Y - 1); ++Y)
		{
			FTile T;
			T.X = X;
			T.Y = Y;
			OutTiles.Add(T);
		}
	}
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	const int32 Cap = GetCapacity();
	if (Cap > 0)
	{
		Items.SetNumZeroed(Cap);
		InvalidateWeight();
	}
}

void UInventoryComponent::OnRegister()
{
	Super::OnRegister();

	const int32 Cap = GetCapacity();
	if (Cap > 0 && Items.Num() != Cap)
	{
		Items.SetNumZeroed(Cap);
		InvalidateWeight();
	}
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsInventoryChanged)
	{
		bIsInventoryChanged = false;
		OnInventoryChanged.Broadcast();
	}

	if (bIsItemUsed)
	{
		bIsItemUsed = false;
		OnItemUsed.Broadcast(ItemUsed);
	}
}

FIntPoint UInventoryComponent::GetEffectiveItemSize(const UItemObject* ItemObject) const
{
	if (!IsValid(ItemObject))
	{
		return FIntPoint(1, 1);
	}

	int32 SX = FMath::Max(1, ItemObject->ItemDetails.Size.X);
	int32 SY = FMath::Max(1, ItemObject->ItemDetails.Size.Y);

	if (ItemObject->Runtime.bIsRotated)
	{
		Swap(SX, SY);
	}

	return FIntPoint(SX, SY);
}

float UInventoryComponent::GetStackWeight(const UItemObject* ItemObject) const
{
	if (!IsValid(ItemObject))
	{
		return 0.f;
	}

	float Weight = 0.f;

	const float UnitW = FMath::Max(0.f, ItemObject->ItemDetails.ItemWeight);
	const int32 Count = FMath::Max(1, ItemObject->Runtime.StackCount);
	Weight += UnitW * (float)Count;

	// Магазин может содержать патроны (которые мы убрали из стака)
	if (ItemObject->IsMagazine() && ItemObject->MagazineAmmoCount > 0)
	{
		const float AmmoUnitW = IsValid(ItemObject->MagazineLoadedAmmoAsset)
			? FMath::Max(0.f, ItemObject->MagazineLoadedAmmoAsset->ItemDetails.ItemWeight)
			: 0.f;

		Weight += AmmoUnitW * (float)FMath::Max(0, ItemObject->MagazineAmmoCount);
	}

	return Weight;
}

bool UInventoryComponent::AreStackCompatible(const UItemObject* A, const UItemObject* B) const
{
	if (!IsValid(A) || !IsValid(B))
	{
		return false;
	}

	if (!A->IsStackable() || !B->IsStackable())
	{
		return false;
	}

	// В нашем проекте самый безопасный ключ — SourceAsset
	return A->SourceAsset == B->SourceAsset;
}
