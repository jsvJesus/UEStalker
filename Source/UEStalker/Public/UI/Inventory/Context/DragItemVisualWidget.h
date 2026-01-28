#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DragItemVisualWidget.generated.h"

class USizeBox;
class UImage;
class UItemObject;

// Мини-виджет только для Drag Visual (без BP).
// Важно: HitTestInvisible, чтобы не блокировать drop targets.
UCLASS()
class UESTALKER_API UDragItemVisualWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Drag")
	void SetupFromItem(UItemObject* Item, float InTileSize);

protected:
	virtual void NativeOnInitialized() override;

private:
	void EnsureTreeBuilt();
	void ApplySetup();

private:
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> RootSizeBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UImage> IconImage = nullptr;

	// pending data (если SetupFromItem вызвали до сборки дерева)
	UPROPERTY(Transient)
	TObjectPtr<UItemObject> PendingItem = nullptr;

	float PendingTileSize = 64.f;
	bool bHasPendingSetup = false;
};