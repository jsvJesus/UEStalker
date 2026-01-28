#include "UI/Inventory/InventorySlotWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/SizeBox.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/InventoryComponent.h"
#include "Materials/MaterialInterface.h"
#include "Items/ItemObject.h"
#include "UI/Inventory/Context/EquipmentDragDropOperation.h"
#include "UI/Inventory/Context/DragItemVisualWidget.h"

bool UInventorySlotWidget::ItemNone() const
{
	return !IsValid(GetItem());
}

UItemObject* UInventorySlotWidget::GetItem() const
{
	return IsValid(EquipmentRef) ? EquipmentRef->GetItemInSlot(SlotId) : nullptr;
}

void UInventorySlotWidget::SetEquipmentRef(UEquipmentComponent* InEquipment)
{
	// --- UNBIND OLD ---
	if (IsValid(EquipmentRef))
	{
		EquipmentRef->OnEquipmentSlotChanged.RemoveDynamic(this, &UInventorySlotWidget::HandleEquipmentChanged);
		EquipmentRef->OnEquipmentActiveSlotChanged.RemoveDynamic(this, &UInventorySlotWidget::HandleActiveSlotChanged);
		EquipmentRef->OnEquipmentSelectedSlotChanged.RemoveDynamic(this, &UInventorySlotWidget::HandleSelectedSlotChanged);
	}
	if (IsValid(InventoryRefCached))
	{
		InventoryRefCached->OnInventoryChanged.RemoveDynamic(this, &UInventorySlotWidget::HandleInventoryChanged);
		InventoryRefCached = nullptr;
	}

	EquipmentRef = InEquipment;

	// --- BIND NEW ---
	if (IsValid(EquipmentRef))
	{
		EquipmentRef->OnEquipmentSlotChanged.AddDynamic(this, &UInventorySlotWidget::HandleEquipmentChanged);
		EquipmentRef->OnEquipmentSelectedSlotChanged.AddDynamic(this, &UInventorySlotWidget::HandleSelectedSlotChanged);

		InventoryRefCached = EquipmentRef->GetInventoryRef();
		if (IsValid(InventoryRefCached))
		{
			InventoryRefCached->OnInventoryChanged.AddDynamic(this, &UInventorySlotWidget::HandleInventoryChanged);
		}
	}

	RefreshSlot(nullptr, FLinearColor::White);
}

void UInventorySlotWidget::UnequipItem(bool bReturnToInventory)
{
	if (!IsValid(EquipmentRef))
	{
		return;
	}

	if (EquipmentRef->UnequipSlot(SlotId, bReturnToInventory))
	{
		OnEquipmentRemoved.Broadcast();
		RefreshSlot(nullptr, FLinearColor::White);
	}
}

bool UInventorySlotWidget::EquipItem(UItemObject* InItem)
{
	if (!IsValid(EquipmentRef))
	{
		return false;
	}

	const bool bOk = EquipmentRef->EquipToSlot(SlotId, InItem, true);
	if (bOk)
	{
		RefreshSlot(nullptr, FLinearColor::White);
	}
	return bOk;
}

UItemObject* UInventorySlotWidget::GetPayload(UDragDropOperation* Operation) const
{
	if (!IsValid(Operation))
	{
		return nullptr;
	}
	return Cast<UItemObject>(Operation->Payload);
}

bool UInventorySlotWidget::CanAcceptItem(UItemObject* InItem) const
{
	return IsValid(EquipmentRef) ? EquipmentRef->CanEquipItemToSlot(InItem, SlotId) : false;
}

void UInventorySlotWidget::RefreshSlot(UMaterialInterface* Material, FLinearColor InColorAndOpacity)
{
	RestoreAfterDrag();
	UItemObject* Item = GetItem();

	if (!IsValid(Item))
	{
		ClearVisual();
		UpdateHighlightFromEquipment();
		return;
	}

	ApplyDesignSize();

	if (IsValid(Icon))
	{
		if (IsValid(Material))
		{
			Icon->SetBrushFromMaterial(Material);
			Icon->SetColorAndOpacity(InColorAndOpacity);
		}
		else
		{
			SetIconFromItem(Item, InColorAndOpacity);
		}
	}

	RefreshDurabilityVisual(Item);
	UpdateHighlightFromEquipment();
}

void UInventorySlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	ApplyDesignSize();
}

void UInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(EquipmentRef))
	{
		EquipmentRef->OnEquipmentSlotChanged.RemoveDynamic(this, &UInventorySlotWidget::HandleEquipmentChanged);
		EquipmentRef->OnEquipmentSlotChanged.AddDynamic(this, &UInventorySlotWidget::HandleEquipmentChanged);

		EquipmentRef->OnEquipmentActiveSlotChanged.RemoveDynamic(this, &UInventorySlotWidget::HandleActiveSlotChanged);
		EquipmentRef->OnEquipmentActiveSlotChanged.AddDynamic(this, &UInventorySlotWidget::HandleActiveSlotChanged);

		EquipmentRef->OnEquipmentSelectedSlotChanged.RemoveDynamic(this, &UInventorySlotWidget::HandleSelectedSlotChanged);

		UInventoryComponent* Inv = EquipmentRef->GetInventoryRef();
		if (InventoryRefCached != Inv)
		{
			if (IsValid(InventoryRefCached))
			{
				InventoryRefCached->OnInventoryChanged.RemoveDynamic(this, &UInventorySlotWidget::HandleInventoryChanged);
			}
			InventoryRefCached = Inv;
		}

		if (IsValid(InventoryRefCached))
		{
			InventoryRefCached->OnInventoryChanged.RemoveDynamic(this, &UInventorySlotWidget::HandleInventoryChanged);
			InventoryRefCached->OnInventoryChanged.AddDynamic(this, &UInventorySlotWidget::HandleInventoryChanged);
		}
	}

	ClearVisual();
	UpdateHighlightFromEquipment();
}

void UInventorySlotWidget::NativeDestruct()
{
	if (IsValid(EquipmentRef))
	{
		EquipmentRef->OnEquipmentSlotChanged.RemoveDynamic(this, &UInventorySlotWidget::HandleEquipmentChanged);
		EquipmentRef->OnEquipmentActiveSlotChanged.RemoveDynamic(this, &UInventorySlotWidget::HandleActiveSlotChanged);
	}

	if (IsValid(InventoryRefCached))
	{
		InventoryRefCached->OnInventoryChanged.RemoveDynamic(this, &UInventorySlotWidget::HandleInventoryChanged);
		InventoryRefCached = nullptr;
	}

	Super::NativeDestruct();
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

FReply UInventorySlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		UnequipItem(true); // вернёт в инвентарь (если есть место)
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation
)
{
	OutOperation = nullptr;

	if (!IsValid(EquipmentRef))
	{
		return;
	}

	UItemObject* Item = GetItem();
	if (!IsValid(Item))
	{
		return;
	}

	if (EquipmentRef->IsSlotLocked(SlotId))
	{
		return;
	}

	// ВАЖНО: пока тянем предмет из слота, этот слот НЕ должен перекрывать hit-test
	CachedVisibility = GetVisibility();
	bDragInProgress = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);

	UEquipmentDragDropOperation* Op = Cast<UEquipmentDragDropOperation>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(UEquipmentDragDropOperation::StaticClass())
	);
	if (!IsValid(Op))
	{
		return;
	}

	Op->Payload = Item;

	// Drag Visual: отдельный виджет, который не блокирует drop targets
	UDragItemVisualWidget* DragVisual = nullptr;
	if (APlayerController* PC = GetOwningPlayer())
	{
		DragVisual = CreateWidget<UDragItemVisualWidget>(PC, UDragItemVisualWidget::StaticClass());
		if (IsValid(DragVisual))
		{
			DragVisual->SetupFromItem(Item, TileSize);
		}
	}

	Op->DefaultDragVisual = IsValid(DragVisual) ? Cast<UWidget>(DragVisual) : Cast<UWidget>(this);
	Op->Pivot = EDragPivot::CenterCenter;
	Op->Offset = FVector2D(0.f, 0.f);

	Op->SourceEquipment = EquipmentRef;
	Op->SourceSlotId = SlotId;

	OutOperation = Op;
}

bool UInventorySlotWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation
)
{
	UItemObject* PayloadItem = GetPayload(InOperation);
	if (!IsValid(PayloadItem))
	{
		if (IsValid(EquipmentRef))
		{
			EquipmentRef->ClearActiveSlot();
		}
		return false;
	}

	// Если в слоте уже есть предмет — сначала попробуем "применить" Payload к нему (Ammo->Mag, Mag->Weapon)
	if (IsValid(EquipmentRef) && IsValid(InventoryRefCached))
	{
		UItemObject* TargetItem = EquipmentRef->GetItemInSlot(SlotId);
		if (IsValid(TargetItem) && TargetItem != PayloadItem)
		{
			int32 AppliedCount = 0;
			if (InventoryRefCached->TryApplyItemToItem(PayloadItem, TargetItem, 0, AppliedCount))
			{
				if (IsValid(EquipmentRef))
				{
					EquipmentRef->ClearActiveSlot();
				}
				UpdateHighlightFromEquipment();
				return true;
			}
		}
	}

	// если положили к нам — EquipmentComponent сам:
	// - снимет предмет из другого слота (если он оттуда)
	// - уберёт из Inventory (если он оттуда)
	const bool bOk = EquipItem(PayloadItem);

	if (IsValid(EquipmentRef))
	{
		EquipmentRef->ClearActiveSlot();
	}
	UpdateHighlightFromEquipment();
	
	return bOk;
}

void UInventorySlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	if (!IsValid(EquipmentRef))
	{
		return;
	}

	UItemObject* PayloadItem = GetPayload(InOperation);
	EquipmentRef->ActivateSlot(SlotId, PayloadItem);
	UpdateHighlightFromEquipment();
}

void UInventorySlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	if (!IsValid(EquipmentRef))
	{
		return;
	}

	EquipmentRef->ClearActiveSlot();
	UpdateHighlightFromEquipment();
}

void UInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	RestoreAfterDrag();

	if (IsValid(EquipmentRef))
	{
		EquipmentRef->ClearActiveSlot();
	}
	UpdateHighlightFromEquipment();
}

void UInventorySlotWidget::HandleEquipmentChanged(EEquipmentSlotId ChangedSlot, UItemObject* NewItem)
{
	if (ChangedSlot != SlotId)
	{
		return;
	}

	RestoreAfterDrag();
	RefreshSlot(nullptr, FLinearColor::White);
}

void UInventorySlotWidget::HandleActiveSlotChanged()
{
	// ActiveSlot/PrevSlot может поменяться без Local drag events (например, когда дроп завершился)
	UpdateHighlightFromEquipment();
}

void UInventorySlotWidget::ApplySizeFromItem(UItemObject* Item)
{
	if (!IsValid(BackGroundSizeBox) || !IsValid(Item))
	{
		return;
	}

	FItemSize Dimensions;
	Item->GetDimensions(Dimensions);

	const float W = FMath::Max(1, Dimensions.X) * TileSize;
	const float H = FMath::Max(1, Dimensions.Y) * TileSize;

	BackGroundSizeBox->SetWidthOverride(W);
	BackGroundSizeBox->SetHeightOverride(H);
}

void UInventorySlotWidget::SetIconFromItem(UItemObject* Item, FLinearColor InColorAndOpacity)
{
	if (!IsValid(Icon) || !IsValid(Item))
	{
		return;
	}

	if (UTexture2D* Tex = Item->ItemDetails.ItemIcon)
	{
		Icon->SetBrushFromTexture(Tex, true);
		Icon->SetColorAndOpacity(InColorAndOpacity);
	}
	else
	{
		FSlateBrush Empty;
		Icon->SetBrush(Empty);
		Icon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
	}
}

void UInventorySlotWidget::ClearVisual()
{
	ApplyDesignSize();

	if (IsValid(Icon))
	{
		FSlateBrush Empty;
		Icon->SetBrush(Empty);
		Icon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
	}

	if (IsValid(DurabilityBar))
	{
		DurabilityBar->SetVisibility(ESlateVisibility::Collapsed);
		DurabilityBar->SetPercent(1.f);
	}
}

void UInventorySlotWidget::UpdateHighlightFromEquipment()
{
	if (!IsValid(BackgroundSlot) || !IsValid(EquipmentRef))
	{
		return;
	}

	// Normal
	FLinearColor C = FLinearColor::White;

	// Blocked = тёмно-серый
	if (EquipmentRef->IsSlotBlocked(SlotId))
	{
		C = FLinearColor(0.15f, 0.15f, 0.15f, 1.f);
	}
	// Active (can drop) = серый
	else if (EquipmentRef->ActiveSlot == SlotId)
	{
		C = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);
	}
	// Prev (cannot drop) = красный
	else if (EquipmentRef->PrevSlot == SlotId)
	{
		C = FLinearColor(1.f, 0.f, 0.f, 1.f);
	}
	else if (EquipmentRef->SelectedSlot == SlotId)
	{
		C = FLinearColor(0.0f, 0.65f, 1.0f, 1.f);
	}

	BackgroundSlot->SetBrushColor(C);
}

void UInventorySlotWidget::ApplyDesignSize()
{
	if (!IsValid(BackGroundSizeBox))
	{
		return;
	}

	const float W = (Width  > 0.f) ? Width  : TileSize;
	const float H = (Height > 0.f) ? Height : TileSize;

	BackGroundSizeBox->SetWidthOverride(W);
	BackGroundSizeBox->SetHeightOverride(H);
}

void UInventorySlotWidget::RefreshDurabilityVisual(UItemObject* Item)
{
	if (!IsValid(DurabilityBar))
	{
		return;
	}

	if (!IsValid(Item) || !Item->DurabilityConfig.bHasDurability || Item->DurabilityConfig.MaxDurability <= KINDA_SMALL_NUMBER)
	{
		DurabilityBar->SetVisibility(ESlateVisibility::Collapsed);
		DurabilityBar->SetPercent(1.f);
		return;
	}

	DurabilityBar->SetVisibility(ESlateVisibility::Visible);
	DurabilityBar->SetPercent(CalcDurability(Item));
}

float UInventorySlotWidget::CalcDurability(const UItemObject* Item)
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

void UInventorySlotWidget::HandleInventoryChanged()
{
	RefreshDurabilityVisual(GetItem());
}

void UInventorySlotWidget::RestoreAfterDrag()
{
	if (!bDragInProgress)
	{
		return;
	}

	bDragInProgress = false;
	SetVisibility(CachedVisibility);
}

void UInventorySlotWidget::HandleSelectedSlotChanged(EEquipmentSlotId NewSelectedSlot)
{
	UpdateHighlightFromEquipment();
}
