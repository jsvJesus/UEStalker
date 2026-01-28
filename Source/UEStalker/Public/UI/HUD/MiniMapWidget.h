#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MiniMapWidget.generated.h"

class UImage;

UCLASS()
class UESTALKER_API UMiniMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UImage> MiniMap = nullptr;
};