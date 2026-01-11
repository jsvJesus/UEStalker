#include "UI/Inventory/InventoryWidget.h"
#include "UI/Inventory/InventoryGridWidget.h"
#include "UI/Inventory/InventorySlotWidget.h"
#include "UI/Inventory/Context/DropAreaWidget.h"
#include "UI/Inventory/Context/EquipmentDragDropOperation.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/InventoryComponent.h"
#include "Components/TextBlock.h"
#include "Components/EquipmentComponent.h"
#include "Items/ItemObject.h"

void UInventoryWidget::InitializeWidget(UInventoryComponent* InInventoryComponent, float InTileSize)
{
	InventoryComponent = InInventoryComponent;
	TileSize = InTileSize;

	SetupWithInventory();
}

bool UInventoryWidget::TryAutoEquipItem(UItemObject* ItemObject)
{
		if (!IsValid(ItemObject) || !IsValid(InventoryComponent))
	{
		return false;
	}

	UEquipmentComponent* EquipComp = EquipmentComponent.Get();
	if (!IsValid(EquipComp))
	{
		AActor* Owner = InventoryComponent->GetOwner();
		EquipComp = Owner ? Owner->FindComponentByClass<UEquipmentComponent>() : nullptr;
		EquipmentComponent = EquipComp;
	}
	if (!IsValid(EquipComp))
	{
		return false;
	}

	const EItemCategory Cat = ItemObject->ItemDetails.ItemCategory;
	const EItemSubCategory Sub = ItemObject->ItemDetails.ItemSubCategory;

	TArray<EEquipmentSlotId> Candidates;

	switch (Cat)
	{
		case EItemCategory::ItemCat_Armor:
			Candidates = { EEquipmentSlotId::ArmorSlot };
			break;
		case EItemCategory::ItemCat_Helmet:
			Candidates = { EEquipmentSlotId::HelmetSlot };
			break;
		case EItemCategory::ItemCat_Backpack:
			Candidates = { EEquipmentSlotId::BackpackSlot };
			break;

		case EItemCategory::ItemCat_Weapons:
			if (Sub == EItemSubCategory::ItemSubCat_Weapons_HG)
			{
				Candidates = { EEquipmentSlotId::PistolSlot };
			}
			else if (Sub == EItemSubCategory::ItemSubCat_Weapons_Knife)
			{
				Candidates = { EEquipmentSlotId::KnifeSlot };
			}
			else if (Sub == EItemSubCategory::ItemSubCat_Weapons_Grenade)
			{
				Candidates = { EEquipmentSlotId::GrenadePrimarySlot, EEquipmentSlotId::GrenadeSecondarySlot };
			}
			else
			{
				Candidates = { EEquipmentSlotId::PrimaryWeaponSlot, EEquipmentSlotId::SecondaryWeaponSlot };
			}
			break;

		case EItemCategory::ItemCat_UsableItems:
			// Devices by default
			Candidates = { EEquipmentSlotId::DeviceSlot1, EEquipmentSlotId::DeviceSlot2, EEquipmentSlotId::DeviceSlot3 };
			break;

		default:
			break;
	}

	// Artefacts / Modules
	if (Candidates.Num() == 0)
	{
		if (Sub == EItemSubCategory::ItemSubCat_Items_Artefacts || Sub == EItemSubCategory::ItemSubCat_Items_Modules)
		{
			Candidates = {
				EEquipmentSlotId::ItemSlot1,
				EEquipmentSlotId::ItemSlot2,
				EEquipmentSlotId::ItemSlot3,
				EEquipmentSlotId::ItemSlot4,
				EEquipmentSlotId::ItemSlot5
			};
		}
		// Quick slots
		else if (Sub == EItemSubCategory::ItemSubCat_Items_Food ||
				 Sub == EItemSubCategory::ItemSubCat_Items_Water ||
				 Sub == EItemSubCategory::ItemSubCat_Items_Medicine)
		{
			Candidates = {
				EEquipmentSlotId::QuickSlot1,
				EEquipmentSlotId::QuickSlot2,
				EEquipmentSlotId::QuickSlot3,
				EEquipmentSlotId::QuickSlot4
			};
		}
	}

	for (const EEquipmentSlotId CandSlot : Candidates)
	{
		if (EquipComp->EquipToSlot(CandSlot, ItemObject, true))
		{
			return true;
		}
	}

	return false;
}

void UInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetupWithInventory();
}

bool UInventoryWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation
)
{
	if (!IsValid(InOperation) || !IsValid(InventoryComponent))
	{
		return false;
	}

	// Payload -> ItemObject
	UItemObject* ItemObject = Cast<UItemObject>(InOperation->Payload);
	if (!IsValid(ItemObject))
	{
		return false;
	}

	// На всякий — снимаем подсветку с экипировочных слотов (если дроп завершился)
	if (IsValid(EquipmentComponent))
	{
		EquipmentComponent->ClearActiveSlot();
	}

	// TryAddItem
	const bool bAdded = InventoryComponent->TryAddItem(ItemObject);
	
	if (bAdded)
	{
		if (UEquipmentDragDropOperation* EquipOp = Cast<UEquipmentDragDropOperation>(InOperation))
		{
			if (IsValid(EquipOp->SourceEquipment))
			{
				EquipOp->SourceEquipment->UnequipSlot(EquipOp->SourceSlotId, false);
			}
		}
	}
	
	return bAdded;
}

void UInventoryWidget::SetupWithInventory()
{
	if (bRuntimeInitialized)
	{
		return;
	}

	if (!IsValid(InventoryComponent))
	{
		return;
	}

	AActor* Owner = InventoryComponent->GetOwner();
	UEquipmentComponent* EquipComp = Owner ? Owner->FindComponentByClass<UEquipmentComponent>() : nullptr;

	EquipmentComponent = EquipComp;
	if (IsValid(EquipmentComponent))
	{
		EquipmentComponent->OnEquipmentSlotChanged.AddDynamic(this, &UInventoryWidget::OnEquipmentSlotChanged);
	}

	auto BindSlot = [&](UInventorySlotWidget* EquipSlot, EEquipmentSlotId InSlotId)
	{
		if (!IsValid(EquipSlot))
		{
			return;
		}
		EquipSlot->SlotId = InSlotId;      // важно: сначала SlotId
		EquipSlot->SetEquipmentRef(EquipComp);
	};

	// Outfit
	BindSlot(WBS_Armor,   EEquipmentSlotId::ArmorSlot);
	BindSlot(WBS_Helmet,  EEquipmentSlotId::HelmetSlot);
	BindSlot(WBS_Backpack,EEquipmentSlotId::BackpackSlot);

	// Weapons
	BindSlot(WBS_PrimaryWeaponSlot,   EEquipmentSlotId::PrimaryWeaponSlot);
	BindSlot(WBS_SecondaryWeaponSlot, EEquipmentSlotId::SecondaryWeaponSlot);
	BindSlot(WBS_PistolSlot,          EEquipmentSlotId::PistolSlot);
	BindSlot(WBS_KnifeSlot,           EEquipmentSlotId::KnifeSlot);

	// Devices
	BindSlot(WBS_DeviceSlot1, EEquipmentSlotId::DeviceSlot1);
	BindSlot(WBS_DeviceSlot2, EEquipmentSlotId::DeviceSlot2);
	BindSlot(WBS_DeviceSlot3, EEquipmentSlotId::DeviceSlot3);

	// Grenades
	BindSlot(WBS_GrenadePrimarySlot,   EEquipmentSlotId::GrenadePrimarySlot);
	BindSlot(WBS_GrenadeSecondarySlot, EEquipmentSlotId::GrenadeSecondarySlot);

	// Modules / Artefacts
	BindSlot(WBS_ItemSlot1, EEquipmentSlotId::ItemSlot1);
	BindSlot(WBS_ItemSlot2, EEquipmentSlotId::ItemSlot2);
	BindSlot(WBS_ItemSlot3, EEquipmentSlotId::ItemSlot3);
	BindSlot(WBS_ItemSlot4, EEquipmentSlotId::ItemSlot4);
	BindSlot(WBS_ItemSlot5, EEquipmentSlotId::ItemSlot5);

	// Quick
	BindSlot(WBS_QuickSlot1, EEquipmentSlotId::QuickSlot1);
	BindSlot(WBS_QuickSlot2, EEquipmentSlotId::QuickSlot2);
	BindSlot(WBS_QuickSlot3, EEquipmentSlotId::QuickSlot3);
	BindSlot(WBS_QuickSlot4, EEquipmentSlotId::QuickSlot4);

	bRuntimeInitialized = true;

	// Initialize Grid
	if (IsValid(WBGridInventory))
	{
		WBGridInventory->InitializeGrid(InventoryComponent, TileSize, this);
	}

	// SetInventoryRef (DropArea)
	if (IsValid(WBDropArea))
	{
		WBDropArea->SetInventoryRef(InventoryComponent);
	}

	// Bind OnInventoryChanged -> OnInventoryChangedEvent
	InventoryComponent->OnInventoryChanged.AddDynamic(this, &UInventoryWidget::OnInventoryChangedEvent);

	// первичное обновление
	OnInventoryChangedEvent();
}

void UInventoryWidget::OnEquipmentSlotChanged(EEquipmentSlotId SlotId, UItemObject* Item)
{
	// Equipment changed -> invalidate cached weight and refresh UI
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->InvalidateWeight();
	}
	OnInventoryChangedEvent();
}

FEventReply UInventoryWidget::HandleBorderMouseDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	return UWidgetBlueprintLibrary::Handled();
}

void UInventoryWidget::OnInventoryChangedEvent()
{
	if (IsValid(WBGridInventory))
	{
		WBGridInventory->Refresh();
	}

	if (!IsValid(InventoryComponent))
	{
		return;
	}

	const float Weight = InventoryComponent->GetTotalWeight();
	const float MaxWeight = InventoryComponent->GetMaxCarryWeight();

	// Формат 1 знак после запятой (можно поменять на "%.0f" если нужно без дробей)
	if (IsValid(TextWeight))
	{
		TextWeight->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Weight)));
	}

	if (IsValid(TextMaxWeight))
	{
		if (MaxWeight <= 0.f)
		{
			TextMaxWeight->SetText(FText::FromString(TEXT("∞"))); // если 0 = без лимита
		}
		else
		{
			TextMaxWeight->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), MaxWeight)));
		}
	}
}
