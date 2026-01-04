#include "UI/Inventory/InventorySlotWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Items/ItemObject.h"
#include "Components/InventoryComponent.h"
#include "Sound/SoundBase.h"

bool UInventorySlotWidget::ItemNone() const
{
	return !IsValid(ItemObject);
}

UItemObject* UInventorySlotWidget::GetItem() const
{
	return IsValid(ItemObject) ? ItemObject : nullptr;
}

void UInventorySlotWidget::UnequipItem(int32 Operation /*=0*/)
{
	if (!IsValid(ItemObject))
	{
		return;
	}

	// Play equip sound
	if (USoundBase* Snd = ItemObject->GetEquipmentSound())
	{
		APawn* Pawn = GetOwningPlayerPawn();
		const FVector Loc = IsValid(Pawn) ? Pawn->GetActorLocation() : FVector::ZeroVector;
		UGameplayStatics::PlaySoundAtLocation(this, Snd, Loc);
	}

	// Сняли предмет
	ItemObject = nullptr;

	// Визуально очистили
	ClearVisual();

	// Событие
	OnEquipmentRemoved.Broadcast();
}

bool UInventorySlotWidget::EquipItem(UItemObject* InItem)
{
	if (bLocked)
	{
		return false;
	}

	if (!IsValid(InItem))
	{
		return false;
	}

	// Уже занято
	if (IsValid(ItemObject))
	{
		return false;
	}

	if (!CanAcceptItem(InItem))
	{
		return false;
	}

	ItemObject = InItem;

	RefreshSlot(nullptr, FLinearColor::White);
	return true;
}

UItemObject* UInventorySlotWidget::GetPayload(UDragDropOperation* Operation) const
{
	if (!IsValid(Operation))
	{
		return nullptr;
	}

	return Cast<UItemObject>(Operation->Payload);
}

void UInventorySlotWidget::RefreshSlot(UMaterialInterface* Material, FLinearColor InColorAndOpacity)
{
	// Нет предмета — очищаем
	if (!IsValid(ItemObject))
	{
		ClearVisual();
		return;
	}

	ApplySizeFromItem();
	
	if (IsValid(Icon))
	{
		if (IsValid(Material))
		{
			Icon->SetBrushFromMaterial(Material);
			Icon->SetColorAndOpacity(InColorAndOpacity);
		}
		else
		{
			// если материал не дали — рисуем иконку предмета
			SetIconFromItem(InColorAndOpacity);
		}
	}
}

FItemOutfitStatsConfig UInventorySlotWidget::GetOutfitStats() const
{
	return IsValid(ItemObject) ? ItemObject->GetOutfitStats() : FItemOutfitStatsConfig{};
}

void UInventorySlotWidget::SetInventoryRef(UInventoryComponent* InventoryComponent)
{
	InventoryRef = InventoryComponent;
}

FInventorySlotRule UInventorySlotWidget::GetSlotType() const
{
	return SlotRule;
}

bool UInventorySlotWidget::CanAcceptItem(UItemObject* InItem) const
{
	if (!IsValid(InItem))
	{
		return false;
	}

	// Универсальный слот
	if (SlotRule.bAcceptAnyItem)
	{
		return true;
	}

	const FMasterItemDetails& D = InItem->ItemDetails;

	// Category
	if (SlotRule.AllowedCategory != EItemCategory::ItemCat_None &&
		D.ItemCategory != SlotRule.AllowedCategory)
	{
		return false;
	}

	// SubCategory
	if (SlotRule.AllowedSubCategory != EItemSubCategory::ItemSubCat_None &&
		D.ItemSubCategory != SlotRule.AllowedSubCategory)
	{
		return false;
	}

	// AmmoType
	if (SlotRule.AllowedAmmoType != EAmmoType::AmmoType_None &&
		D.AmmoType != SlotRule.AllowedAmmoType)
	{
		return false;
	}

	return true;
}

void UInventorySlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// EventPreConstruct: Width/Height -> SizeBox + сохранить Rotated
	if (IsValid(BackGroundSizeBox))
	{
		BackGroundSizeBox->SetWidthOverride(Width);
		BackGroundSizeBox->SetHeightOverride(Height);
	}
}

void UInventorySlotWidget::SetSlotType(const FInventorySlotRule& InSlotRule)
{
	SlotRule = InSlotRule;
}

void UInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ClearVisual();
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	OutOperation = nullptr;

	if (bLocked || !IsValid(ItemObject))
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

	// Bind Event to OnDrop -> UnequipItem
	Op->OnDrop.AddDynamic(this, &UInventorySlotWidget::HandleOperationDrop);

	OutOperation = Op;
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (bLocked)
	{
		return false;
	}

	UItemObject* PayloadItem = GetPayload(InOperation);
	if (!IsValid(PayloadItem))
	{
		return false;
	}

	// уже занято
	if (IsValid(ItemObject))
	{
		return false;
	}

	if (!CanAcceptItem(PayloadItem))
	{
		return false;
	}

	return EquipItem(PayloadItem);
}

void UInventorySlotWidget::HandleOperationDrop(UDragDropOperation* Operation)
{
	// Любые drop операции, начатой из этого слота -> снять предмет со слота
	UnequipItem(0);
}

void UInventorySlotWidget::ApplySizeFromItem()
{
	if (!IsValid(BackGroundSizeBox) || !IsValid(ItemObject))
	{
		return;
	}

	// Берём размер из MasterItemStructs: ItemDetails.Size (и учитываем rotation через GetDimensions)
	FItemSize Dimensions;
	ItemObject->GetDimensions(Dimensions);

	const float W = FMath::Max(1, Dimensions.X) * TileSize;
	const float H = FMath::Max(1, Dimensions.Y) * TileSize;

	BackGroundSizeBox->SetWidthOverride(W);
	BackGroundSizeBox->SetHeightOverride(H);
}

void UInventorySlotWidget::SetIconFromItem(FLinearColor InColorAndOpacity)
{
	if (!IsValid(Icon) || !IsValid(ItemObject))
	{
		return;
	}

	if (UTexture2D* Tex = ItemObject->ItemDetails.ItemIcon)
	{
		Icon->SetBrushFromTexture(Tex, true);
		Icon->SetColorAndOpacity(InColorAndOpacity);
	}
	else
	{
		// нет иконки — делаем прозрачным
		FSlateBrush Empty;
		Icon->SetBrush(Empty);
		Icon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
	}
}

void UInventorySlotWidget::ClearVisual()
{
	// Пустой слот по умолчанию 1x1
	if (IsValid(BackGroundSizeBox))
	{
		BackGroundSizeBox->SetWidthOverride(TileSize);
		BackGroundSizeBox->SetHeightOverride(TileSize);
	}

	if (IsValid(Icon))
	{
		FSlateBrush Empty;
		Icon->SetBrush(Empty);
		Icon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
	}
}
