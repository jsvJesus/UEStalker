#include "UI/Inventory/InventoryGridWidget.h"
#include "UI/Inventory/InventoryItemWidget.h"
#include "UI/Inventory/InventoryWidget.h"
#include "UI/Inventory/Context/EquipmentDragDropOperation.h"
#include "UI/Inventory/Context/InventoryItemDragDropOperation.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/InventoryComponent.h"
#include "Components/EquipmentComponent.h"
#include "Items/ItemObject.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

UItemObject* UInventoryGridWidget::GetPayload(UDragDropOperation* DragDropOperation) const
{
	if (!IsValid(DragDropOperation))
	{
		return nullptr;
	}

	// В Payload мы кладём напрямую UItemObject*
	return Cast<UItemObject>(DragDropOperation->Payload);
}

FEventReply UInventoryGridWidget::HandleGridBorderMouseDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	return UWidgetBlueprintLibrary::Handled();
}

bool UInventoryGridWidget::IsRoomAvailableForPayload(const FInventoryItemPayload& Payload, const UDragDropOperation* Operation) const
{
	if (!IsValid(InventoryComponent) || !IsValid(Payload.ItemObject))
	{
		return false;
	}

	const FTile TopLeftTile(DraggedItemTopLeftTileX, DraggedItemTopLeftTileY);
	const int32 TopLeftIndex = InventoryComponent->TileToIndex(TopLeftTile);
	if (TopLeftIndex == INDEX_NONE)
	{
		return false;
	}

	// Если перетаскиваем предмет внутри того же инвентаря — разрешаем перекрывать его старые клетки
	if (const UInventoryItemDragDropOperation* InvOp = Cast<UInventoryItemDragDropOperation>(Operation))
	{
		if (InvOp->SourceInventory == InventoryComponent)
		{
			return InventoryComponent->IsRoomAvailableForMove(Payload.ItemObject, TopLeftIndex);
		}
	}

	return InventoryComponent->IsRoomAvailable(Payload.ItemObject, TopLeftIndex);
}

void UInventoryGridWidget::GetMousePositionInTile(FVector2D& LocalPos, bool& bRight, bool& bDown) const
{
	LocalPos = FVector2D::ZeroVector;
	bRight = false;
	bDown  = false;

	if (!IsValid(GridCanvasPanel))
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!IsValid(PC))
	{
		return;
	}

	// Абсолютная позиция мыши (viewport)
	FVector2D AbsMouse = FVector2D::ZeroVector;
	if (!UWidgetLayoutLibrary::GetMousePositionScaledByDPI(PC, AbsMouse.X, AbsMouse.Y))
	{
		return;
	}

	// Переводим в локальные координаты CanvasPanel
	const FGeometry& Geo = GridCanvasPanel->GetCachedGeometry();
	LocalPos = USlateBlueprintLibrary::AbsoluteToLocal(Geo, AbsMouse);

	// Определяем положение внутри тайла
	const float ModX = FMath::Fmod(LocalPos.X, TileSize);
	const float ModY = FMath::Fmod(LocalPos.Y, TileSize);

	bRight = (ModX > (TileSize * 0.5f));
	bDown  = (ModY > (TileSize * 0.5f));
}

void UInventoryGridWidget::OnItemUsed(UItemObject* ItemObject)
{
	if (!IsValid(InventoryComponent) || !IsValid(ItemObject))
	{
		return;
	}

	// DoubleClick: first try auto-equip (Armor/Helmet/Backpack/Weapons/etc)
	if (IsValid(WBInventory) && WBInventory->TryAutoEquipItem(ItemObject))
	{
		return;
	}

	// Only use consumables on double click, avoid deleting weapons/armor by accident
	const EItemCategory Cat = ItemObject->ItemDetails.ItemCategory;
	const EItemSubCategory Sub = ItemObject->ItemDetails.ItemSubCategory;

	const bool bIsConsumable =
		(Cat == EItemCategory::ItemCat_UsableItems) ||
		(Sub == EItemSubCategory::ItemSubCat_Items_Food) ||
		(Sub == EItemSubCategory::ItemSubCat_Items_Water) ||
		(Sub == EItemSubCategory::ItemSubCat_Items_Medicine);

	if (bIsConsumable)
	{
		InventoryComponent->UseItem(ItemObject);
	}
}

void UInventoryGridWidget::OnItemRemoved(UItemObject* ItemObject)
{
	if (!IsValid(InventoryComponent) || !IsValid(ItemObject))
	{
		return;
	}

	InventoryComponent->RemoveItem(ItemObject);
}

void UInventoryGridWidget::CreateLineSegments()
{
	Lines.Empty();

	if (!IsValid(InventoryComponent))
	{
		return;
	}

	const int32 Cols = InventoryComponent->Columns;
	const int32 Rows = InventoryComponent->Rows;

	// Размер грида в пикселях
	const float W = Cols * TileSize;
	const float H = Rows * TileSize;

	// Выставляем размер SizeBox (1x1 = 64x64, значит общий размер = Cols*64 / Rows*64)
	if (IsValid(GridSizeBox))
	{
		GridSizeBox->SetWidthOverride(W);
		GridSizeBox->SetHeightOverride(H);
	}

	// Вертикальные линии (X)
	for (int32 X = 0; X <= Cols; ++X)
	{
		FLine2D L;
		L.Start = FVector2D(X * TileSize, 0.f);
		L.End   = FVector2D(X * TileSize, H);
		Lines.Add(L);
	}

	// Горизонтальные линии (Y)
	for (int32 Y = 0; Y <= Rows; ++Y)
	{
		FLine2D L;
		L.Start = FVector2D(0.f, Y * TileSize);
		L.End   = FVector2D(W,  Y * TileSize);
		Lines.Add(L);
	}
}

void UInventoryGridWidget::InitializeGrid(UInventoryComponent* InInventoryComponent, float InTileSize,
	UInventoryWidget* InWBInventory)
{
	InventoryComponent = InInventoryComponent;
	WBInventory = InWBInventory;
	TileSize = InTileSize;

	CreateLineSegments();
	Refresh();

	if (IsValid(InventoryComponent))
	{
		// Чтобы не плодить подписки
		InventoryComponent->OnInventoryChanged.RemoveAll(this);
		InventoryComponent->OnInventoryChanged.AddDynamic(this, &UInventoryGridWidget::Refresh);
	}
}

void UInventoryGridWidget::Refresh()
{
	if (!IsValid(GridCanvasPanel) || !IsValid(InventoryComponent))
	{
		return;
	}

	GridCanvasPanel->ClearChildren();

	// Карта уникальных предметов -> их TopLeft tile
	TMap<UItemObject*, FTile> ItemToTile;
	GetItemTileMap(ItemToTile);

	APlayerController* PC = GetOwningPlayer();

	for (const TPair<UItemObject*, FTile>& Pair : ItemToTile)
	{
		UItemObject* Item = Pair.Key;
		const FTile& Tile = Pair.Value;
		if (!IsValid(Item))
		{
			continue;
		}

		// Создаем виджет предмета
		UClass* UseClass = ItemWidgetClass ? ItemWidgetClass.Get() : UInventoryItemWidget::StaticClass();
		UInventoryItemWidget* ItemWidget = CreateWidget<UInventoryItemWidget>(PC, UseClass);
		if (!IsValid(ItemWidget))
		{
			continue;
		}

		ItemWidget->ItemObject = Item;
		ItemWidget->TileSize = TileSize;
		ItemWidget->InventoryComponent = InventoryComponent;
		ItemWidget->Refresh();

		// Подписываемся на события Use/Delete из item widget
		ItemWidget->OnUseSelectedItem.RemoveAll(this);
		ItemWidget->OnDeleteSelectedItem.RemoveAll(this);
		ItemWidget->OnUseSelectedItem.AddDynamic(this, &UInventoryGridWidget::OnItemUsed);
		ItemWidget->OnDeleteSelectedItem.AddDynamic(this, &UInventoryGridWidget::OnItemRemoved);

		// Добавляем в CanvasPanel
		UCanvasPanelSlot* GridSlot = Cast<UCanvasPanelSlot>(GridCanvasPanel->AddChild(ItemWidget));
		if (!GridSlot)
		{
			continue;
		}

		GridSlot->SetAutoSize(true);
		GridSlot->SetPosition(FVector2D(Tile.X * TileSize, Tile.Y * TileSize));
	}
}

bool UInventoryGridWidget::GetTopLeftTileForItem(UItemObject* ItemObject, FTile& OutTile) const
{
	OutTile = FTile(-1, -1);

	if (!IsValid(InventoryComponent) || !IsValid(ItemObject))
	{
		return false;
	}

	// Items — 1D массив клеток (row-major). Первая найденная клетка будет TopLeft.
	const TArray<TObjectPtr<UItemObject>>& Cells = InventoryComponent->Items;

	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		if (Cells[Index] == ItemObject)
		{
			OutTile = InventoryComponent->IndexToTile(Index);
			return true;
		}
	}

	return false;
}

void UInventoryGridWidget::GetItemTileMap(TMap<UItemObject*, FTile>& OutMap) const
{
	OutMap.Empty();

	if (!IsValid(InventoryComponent))
	{
		return;
	}

	// Items — массив клеток (1D)
	const TArray<TObjectPtr<UItemObject>>& Cells = InventoryComponent->Items;

	// Чтобы добавить каждый предмет только 1 раз (в его TopLeft)
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		UItemObject* Item = Cells[Index].Get();
		if (!IsValid(Item))
		{
			continue;
		}

		// Если уже добавляли этот Item — пропускаем (значит это не его TopLeft клетка)
		if (OutMap.Contains(Item))
		{
			continue;
		}

		const FTile Tile = InventoryComponent->IndexToTile(Index);
		OutMap.Add(Item, Tile);
	}
}

void UInventoryGridWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	const int32 Cols = IsValid(InventoryComponent) ? InventoryComponent->Columns : PreviewColumns;
	const int32 Rows = IsValid(InventoryComponent) ? InventoryComponent->Rows : PreviewRows;

	if (IsValid(GridSizeBox))
	{
		GridSizeBox->SetWidthOverride(Cols * TileSize);
		GridSizeBox->SetHeightOverride(Rows * TileSize);
	}
}

void UInventoryGridWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	bDrawDropLocation = true;
	Invalidate(EInvalidateWidget::Paint);
}

void UInventoryGridWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	bDrawDropLocation = false;
	Invalidate(EInvalidateWidget::Paint);
}

bool UInventoryGridWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);

	if (!bDrawDropLocation || !IsValid(InventoryComponent) || !IsValid(InOperation))
	{
		return false;
	}

	UItemObject* Item = GetPayload(InOperation);
	if (!IsValid(Item))
	{
		return false;
	}

	// Если дроп пришёл от equipment-drag (или когда подсветка могла залипнуть) — сбросим её
	if (UEquipmentDragDropOperation* EquipOp = Cast<UEquipmentDragDropOperation>(InOperation))
	{
		if (IsValid(EquipOp->SourceEquipment))
		{
			EquipOp->SourceEquipment->ClearActiveSlot();
		}
	}

	// Локальные координаты курсора внутри грида
	MousePosition = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());

	// Положение внутри текущего тайла
	const float ModX = FMath::Fmod(MousePosition.X, TileSize);
	const float ModY = FMath::Fmod(MousePosition.Y, TileSize);
	const bool bRight = (ModX > (TileSize * 0.5f));
	const bool bDown  = (ModY > (TileSize * 0.5f));

	// Размер предмета (в тайлах)
	FItemSize Dims;
	Item->GetDimensions(Dims);
	const int32 DimsX = FMath::Max(1, Dims.X);
	const int32 DimsY = FMath::Max(1, Dims.Y);

	// Клетка под курсором
	const int32 HoverTileX = FMath::FloorToInt(MousePosition.X / TileSize);
	const int32 HoverTileY = FMath::FloorToInt(MousePosition.Y / TileSize);

	// Смещение top-left в зависимости от половины тайла
	const int32 OffsetX = bRight ? (DimsX - 1) : 0;
	const int32 OffsetY = bDown  ? (DimsY - 1) : 0;

	const int32 MaxX = FMath::Max(0, InventoryComponent->Columns - DimsX);
	const int32 MaxY = FMath::Max(0, InventoryComponent->Rows - DimsY);

	DraggedItemTopLeftTileX = FMath::Clamp(HoverTileX - OffsetX, 0, MaxX);
	DraggedItemTopLeftTileY = FMath::Clamp(HoverTileY - OffsetY, 0, MaxY);

	Invalidate(EInvalidateWidget::Paint);
	return true;
}

void UInventoryGridWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Если компонент задан — сразу выставим размеры и линии
	CreateLineSegments();
}

bool UInventoryGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (!IsValid(InventoryComponent))
	{
		return false;
	}

	UItemObject* Item = GetPayload(InOperation);
	if (!IsValid(Item))
	{
		return false;
	}

	FInventoryItemPayload Payload;
	Payload.ItemObject = Item;

	// Move внутри того же инвентаря: сначала временно убираем предмет, чтобы освободить его клетки
	const UInventoryItemDragDropOperation* InvOp = Cast<UInventoryItemDragDropOperation>(InOperation);
	const bool bFromSameInventory = InvOp && (InvOp->SourceInventory == InventoryComponent);
	const int32 SourceTopLeftIndex = bFromSameInventory ? InvOp->SourceTopLeftIndex : INDEX_NONE;

	if (bFromSameInventory)
	{
		InventoryComponent->RemoveItem(Item);
	}

	// Если предмет можно положить именно в DraggedItemTopLeftTileX/Y -> кладём туда
	if (IsRoomAvailableForPayload(Payload, InOperation))
	{
		const int32 TopLeftIndex = InventoryComponent->TileToIndex(FTile(DraggedItemTopLeftTileX, DraggedItemTopLeftTileY));
		if (TopLeftIndex != INDEX_NONE)
		{
			InventoryComponent->AddItemAt(Item, TopLeftIndex);

			// if dragged from EquipmentSlot -> clear it immediately
			if (UEquipmentDragDropOperation* EquipOp = Cast<UEquipmentDragDropOperation>(InOperation))
			{
				if (IsValid(EquipOp->SourceEquipment))
				{
					EquipOp->SourceEquipment->UnequipSlot(EquipOp->SourceSlotId, false);
				}
			}
			
			return true;
		}
	}

	// Если это было перемещение внутри того же инвентаря, а место не найдено — возвращаем обратно
	if (bFromSameInventory)
	{
		if (SourceTopLeftIndex != INDEX_NONE)
		{
			InventoryComponent->AddItemAt(Item, SourceTopLeftIndex);
		}
		else
		{
			InventoryComponent->TryAddItem(Item);
		}
		return true;
	}

	// Иначе -> TryAddItem. Если не удалось -> DropItem в мир.
	const bool bAdded = InventoryComponent->TryAddItem(Item);
	if (!bAdded)
	{
		AActor* OwnerActor = InventoryComponent->GetOwner();
		InventoryComponent->DropItem(OwnerActor, Item, true);
	}

	// если перетащили из EquipmentSlot -> надо снять из слота
	if (UEquipmentDragDropOperation* EquipOp = Cast<UEquipmentDragDropOperation>(InOperation))
	{
		if (IsValid(EquipOp->SourceEquipment))
		{
			EquipOp->SourceEquipment->UnequipSlot(EquipOp->SourceSlotId, false);
		}
	}

	return true;
}

int32 UInventoryGridWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled
) const
{
	// СЕТКА СНАЧАЛА (под предметами)
	if (Lines.Num() > 0)
	{
		// умножим на общий тинт виджета (если вдруг кто-то его меняет)
		const FLinearColor FinalLineColor = GridLineColor * InWidgetStyle.GetColorAndOpacityTint();

		for (const FLine2D& L : Lines)
		{
			TArray<FVector2D> Points;
			Points.Add(L.Start);
			Points.Add(L.End);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				Points,
				ESlateDrawEffect::None,
				FinalLineColor,
				bGridLinesAntialias,
				LineThickness
			);
		}
	}

	// ДЕТИ (иконки/предметы) ПОВЕРХ СЕТКИ
	int32 RetLayer = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId + 1,
		InWidgetStyle,
		bParentEnabled
	);

	// Подсветка места дропа (поверх всего)
	if (bDrawDropLocation)
	{
		UDragDropOperation* CurrentOp = UWidgetBlueprintLibrary::GetDragDroppingContent();
		UItemObject* Item = GetPayload(CurrentOp);

		if (IsValid(Item) && IsValid(InventoryComponent))
		{
			FInventoryItemPayload Payload;
			Payload.ItemObject = Item;

			const bool bCanDrop = IsRoomAvailableForPayload(Payload, CurrentOp);

			FItemSize Dims;
			Item->GetDimensions(Dims);

			const float BoxW = FMath::Max(1, Dims.X) * TileSize;
			const float BoxH = FMath::Max(1, Dims.Y) * TileSize;

			const FVector2D BoxPos(
				DraggedItemTopLeftTileX * TileSize,
				DraggedItemTopLeftTileY * TileSize
			);

			const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
			const FLinearColor Tint = bCanDrop
				? FLinearColor(0.f, 1.f, 0.f, 0.25f)
				: FLinearColor(1.f, 0.f, 0.f, 0.25f);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				RetLayer + 1,
				AllottedGeometry.ToPaintGeometry(BoxPos, FVector2D(BoxW, BoxH)),
				WhiteBrush,
				ESlateDrawEffect::None,
				Tint
			);

			return RetLayer + 1;
		}
	}

	return RetLayer;
}
