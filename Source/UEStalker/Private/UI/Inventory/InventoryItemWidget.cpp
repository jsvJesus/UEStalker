#include "UI/Inventory/InventoryItemWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"
#include "UI/Inventory/Context/InventoryItemDragDropOperation.h"
#include "UI/Inventory/Context/DragItemVisualWidget.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/InventoryComponent.h"
#include "Items/ItemObject.h"

FSlateBrush UInventoryItemWidget::GetIconImage() const
{
	FSlateBrush Brush;

	if (!IsValid(ItemObject))
	{
		return Brush;
	}

	// Размер под фактический dimension (учитывая rotation)
	FItemSize Dim;
	ItemObject->GetDimensions(Dim);

	const float W = FMath::Max(1, Dim.X) * TileSize;
	const float H = FMath::Max(1, Dim.Y) * TileSize;

	UTexture2D* IconTex = nullptr;

	// Если предмет повернут и есть IconRotated — используем её
	if (ItemObject->Runtime.bIsRotated && IsValid(ItemObject->ItemDetails.IconRotated))
	{
		IconTex = ItemObject->ItemDetails.IconRotated;
	}
	else
	{
		IconTex = ItemObject->ItemDetails.ItemIcon;
	}

	if (IsValid(IconTex))
	{
		Brush.SetResourceObject(IconTex);
		Brush.ImageSize = FVector2D(W, H);
	}

	return Brush;
}

void UInventoryItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UInventoryItemWidget::Refresh);
	}
	else
	{
		Refresh();
	}
}

void UInventoryItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
                                                UDragDropOperation*& OutOperation)
{
	OutOperation = nullptr;

	if (!IsValid(ItemObject))
	{
		return;
	}

	UInventoryItemDragDropOperation* Op = Cast<UInventoryItemDragDropOperation>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(UInventoryItemDragDropOperation::StaticClass())
	);
	if (!IsValid(Op))
	{
		return;
	}

	// Payload = ItemObject
	Op->Payload = ItemObject;

	// Drag Visual: дубликат этого же виджета (с BP-дизайном)
	UInventoryItemWidget* DragVisual = nullptr;
	if (APlayerController* PC = GetOwningPlayer())
	{
		DragVisual = CreateWidget<UInventoryItemWidget>(PC, GetClass());
		if (IsValid(DragVisual))
		{
			DragVisual->ItemObject = ItemObject;
			DragVisual->InventoryComponent = InventoryComponent;
			DragVisual->TileSize = TileSize;
			DragVisual->Refresh();
			DragVisual->SetIsEnabled(false);
			DragVisual->SetRenderOpacity(0.85f);
		}
	}

	Op->DefaultDragVisual = IsValid(DragVisual) ? Cast<UWidget>(DragVisual) : Cast<UWidget>(this);
	Op->Pivot = EDragPivot::CenterCenter;
	Op->Offset = FVector2D(0.f, 0.f);

	// Call On Delete Selected Item + RemoveFromParent
	OnDeleteSelectedItem.Broadcast(ItemObject);
	RemoveFromParent();

	OutOperation = Op;
}

bool UInventoryItemWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation
)
{
	if (!IsValid(ItemObject) || !IsValid(InventoryComponent) || !IsValid(InOperation))
	{
		return false;
	}

	UItemObject* Payload = Cast<UItemObject>(InOperation->Payload);
	if (!IsValid(Payload) || Payload == ItemObject)
	{
		return false;
	}

	int32 AppliedCount = 0;
	if (InventoryComponent->TryApplyItemToItem(Payload, ItemObject, 0, AppliedCount))
	{
		// UI обновится через OnInventoryChanged
		return true;
	}

	return false;
}

bool UInventoryItemWidget::NativeOnDragOver(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation
)
{
	if (!IsValid(ItemObject) || !IsValid(InventoryComponent) || !IsValid(InOperation))
	{
		return false;
	}

	UItemObject* Payload = Cast<UItemObject>(InOperation->Payload);
	if (!IsValid(Payload) || Payload == ItemObject)
	{
		return false;
	}

	return InventoryComponent->CanApplyItemToItem(Payload, ItemObject);
}

void UInventoryItemWidget::NativeOnDragCancelled(
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation
)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
}

FReply UInventoryItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// DetectDragIfPressed (LeftMouseButton)
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

FReply UInventoryItemWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (IsValid(ItemObject))
	{
		OnUseSelectedItem.Broadcast(ItemObject);
	}

	return UWidgetBlueprintLibrary::Handled().NativeReply;
}

void UInventoryItemWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	// EventOnMouseEnter (меняем цвет бордера)
	if (IsValid(BackgroundBorder))
	{
		BackgroundBorder->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 1.f));
	}
}

void UInventoryItemWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	// EventOnMouseLeave
	if (IsValid(BackgroundBorder))
	{
		BackgroundBorder->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.5f));
	}
}

void UInventoryItemWidget::ApplySizeFromItem()
{
	if (!IsValid(BackgroundSizeBox) || !IsValid(ItemObject))
	{
		return;
	}

	FItemSize Dim;
	ItemObject->GetDimensions(Dim);

	const float W = FMath::Max(1, Dim.X) * TileSize;
	const float H = FMath::Max(1, Dim.Y) * TileSize;

	BackgroundSizeBox->SetWidthOverride(W);
	BackgroundSizeBox->SetHeightOverride(H);
}

void UInventoryItemWidget::RefreshCountVisual()
{
	if (!IsValid(CountText) || !IsValid(BorderCountText))
	{
		return;
	}

	const FText Txt = GetCountItems();
	CountText->SetText(Txt);

	const bool bShow = !Txt.IsEmpty();
	BorderCountText->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UInventoryItemWidget::RefreshDurabilityVisual()
{
	if (!IsValid(DurabilityBar))
	{
		return;
	}

	if (!IsValid(ItemObject) || !ItemObject->DurabilityConfig.bHasDurability || ItemObject->DurabilityConfig.MaxDurability <= KINDA_SMALL_NUMBER)
	{
		DurabilityBar->SetVisibility(ESlateVisibility::Collapsed);
		DurabilityBar->SetPercent(1.f);
		return;
	}

	DurabilityBar->SetVisibility(ESlateVisibility::Visible);
	DurabilityBar->SetPercent(CalcDurability(ItemObject));
}

float UInventoryItemWidget::CalcDurability(const UItemObject* Item)
{
	if (!IsValid(Item))
	{
		return 0.f;
	}

	if (!Item->DurabilityConfig.bHasDurability)
	{
		return 1.f;
	}

	const float MaxD = Item->DurabilityConfig.MaxDurability;
	if (MaxD <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	return FMath::Clamp(Item->Runtime.CurrDurability / MaxD, 0.f, 1.f);
}

FText UInventoryItemWidget::GetCountItems() const
{
	if (!IsValid(ItemObject))
	{
		return FText::GetEmpty();
	}

	// Magazine: show ammo in mag (0/30)
	if (ItemObject->IsMagazine())
	{
		const int32 Cap = ItemObject->GetMagazineCapacity();
		if (Cap > 0)
		{
			return FText::Format(NSLOCTEXT("Inventory", "MagAmmoFmt", "{0}/{1}"),
				FText::AsNumber(FMath::Max(0, ItemObject->GetMagazineCurrentAmmo())),
				FText::AsNumber(Cap));
		}
		return FText::GetEmpty();
	}

	// Stackable: show only count (even if 1)
	if (ItemObject->IsStackable())
	{
		return FText::AsNumber(FMath::Max(0, ItemObject->Runtime.StackCount));
	}

	return FText::GetEmpty();
}

bool UInventoryItemWidget::RemoveItem()
{
	if (!IsValid(ItemObject))
	{
		return false;
	}

	OnDeleteSelectedItem.Broadcast(ItemObject);
	RemoveFromParent();
	return true;
}

void UInventoryItemWidget::Refresh()
{
	ApplySizeFromItem();
	RefreshCountVisual();
	RefreshDurabilityVisual();

	if (IsValid(ItemImage))
	{
		ItemImage->SetBrush(GetIconImage());
	}
}

void UInventoryItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplySizeFromItem();
	RefreshCountVisual();
	RefreshDurabilityVisual();

	Refresh();

	// если ItemImage использует биндинг на GetIconImage — ок,
	// если нет — выставим руками
	if (IsValid(ItemImage))
	{
		ItemImage->SetBrush(GetIconImage());
	}
}
