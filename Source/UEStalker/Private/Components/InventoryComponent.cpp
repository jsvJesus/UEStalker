#include "Components/InventoryComponent.h"
#include "Items/ItemObject.h"
#include "Items/MasterItemActor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	
	Columns = 8;
	Rows = 11;

	Items.SetNumZeroed(GetCapacity());
}

void UInventoryComponent::SetGridSize(int32 NewColumns, int32 NewRows)
{
	Columns = FMath::Max(1, NewColumns);
	Rows    = FMath::Max(1, NewRows);

	Items.SetNumZeroed(GetCapacity());
	bIsInventoryChanged = true;
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

	bIsInventoryChanged = true;
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

	const int32 Cap = GetCapacity();
	if (Cap <= 0)
	{
		return false;
	}

	if (Items.Num() != Cap)
	{
		Items.SetNumZeroed(Cap);
	}

	for (int32 TopLeftIndex = 0; TopLeftIndex < Items.Num(); ++TopLeftIndex)
	{
		if (IsRoomAvailable(ItemObject, TopLeftIndex))
		{
			AddItemAt(ItemObject, TopLeftIndex);
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
		bIsInventoryChanged = true;
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
	bIsInventoryChanged = true;

	bIsItemUsed = true;
	ItemUsed = ItemObject;

	// Звук использования
	if (USoundBase* UseSound = ItemObject->GetSoundOfUse())
	{
		const FVector Loc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
		UGameplayStatics::PlaySoundAtLocation(this, UseSound, Loc);
	}
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
	}
}

void UInventoryComponent::OnRegister()
{
	Super::OnRegister();

	const int32 Cap = GetCapacity();
	if (Cap > 0 && Items.Num() != Cap)
	{
		Items.SetNumZeroed(Cap);
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
