#include "UI/Inventory/Context/DragItemVisualWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Items/ItemObject.h"

void UDragItemVisualWidget::SetupFromItem(UItemObject* Item, float InTileSize)
{
	PendingItem = Item;
	PendingTileSize = InTileSize;
	bHasPendingSetup = true;

	// если дерево уже собрано — применяем сразу
	if (RootSizeBox && IconImage)
	{
		ApplySetup();
	}
}

void UDragItemVisualWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	EnsureTreeBuilt();

	// Drag visual НЕ должен блокировать drop targets
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (bHasPendingSetup)
	{
		ApplySetup();
	}
}

void UDragItemVisualWidget::EnsureTreeBuilt()
{
	if (RootSizeBox && IconImage)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	// Root
	if (!WidgetTree->RootWidget)
	{
		RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		WidgetTree->RootWidget = RootSizeBox;
	}
	else
	{
		RootSizeBox = Cast<USizeBox>(WidgetTree->RootWidget);
		if (!RootSizeBox)
		{
			RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
			WidgetTree->RootWidget = RootSizeBox;
		}
	}

	// Image
	if (!IconImage)
	{
		IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IconImage"));
		RootSizeBox->SetContent(IconImage);
	}
}

void UDragItemVisualWidget::ApplySetup()
{
	bHasPendingSetup = false;

	if (!IsValid(PendingItem) || !IsValid(RootSizeBox) || !IsValid(IconImage))
	{
		return;
	}

	FItemSize Dim;
	PendingItem->GetDimensions(Dim);

	const float W = FMath::Max(1, Dim.X) * PendingTileSize;
	const float H = FMath::Max(1, Dim.Y) * PendingTileSize;

	RootSizeBox->SetWidthOverride(W);
	RootSizeBox->SetHeightOverride(H);

	UTexture2D* IconTex = PendingItem->ItemDetails.ItemIcon;
	if (PendingItem->Runtime.bIsRotated && IsValid(PendingItem->ItemDetails.IconRotated))
	{
		IconTex = PendingItem->ItemDetails.IconRotated;
	}

	if (IsValid(IconTex))
	{
		IconImage->SetBrushFromTexture(IconTex, true);
		IconImage->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		FSlateBrush Empty;
		IconImage->SetBrush(Empty);
		IconImage->SetColorAndOpacity(FLinearColor(1,1,1,0));
	}
}
