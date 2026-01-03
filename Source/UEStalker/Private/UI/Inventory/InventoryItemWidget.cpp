#include "UI/Inventory/InventoryItemWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
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

	UDragDropOperation* Op = UWidgetBlueprintLibrary::CreateDragDropOperation(UDragDropOperation::StaticClass());
	if (!IsValid(Op))
	{
		return;
	}

	// Payload = ItemObject, DefaultDragVisual = self
	Op->Payload = ItemObject;
	Op->DefaultDragVisual = this;
	Op->Pivot = EDragPivot::CenterCenter;
	Op->Offset = FVector2D(0.f, 0.f);

	// Call On Delete Selected Item + RemoveFromParent
	OnDeleteSelectedItem.Broadcast(ItemObject);
	RemoveFromParent();

	OutOperation = Op;
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

FText UInventoryItemWidget::GetCountItems() const
{
	if (!IsValid(ItemObject))
	{
		return FText::GetEmpty();
	}

	const int32 Count = ItemObject->Runtime.StackCount;

	// показываем только если реально стак (больше 1)
	if (ItemObject->IsStackable() && Count > 1)
	{
		return FText::AsNumber(Count);
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

	Refresh();

	// если ItemImage использует биндинг на GetIconImage — ок,
	// если нет — выставим руками
	if (IsValid(ItemImage))
	{
		ItemImage->SetBrush(GetIconImage());
	}
}
