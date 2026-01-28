#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Character/MasterCharacter.h"
#include "AnimNotifyState_SetLeftHandIKMode.generated.h"

UCLASS(meta=(DisplayName="Set Left Hand IK Mode"))
class UESTALKER_API UAnimNotifyState_SetLeftHandIKMode : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IK")
	ELeftHandIKMode BeginMode = ELeftHandIKMode::ReloadMag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IK")
	ELeftHandIKMode EndMode = ELeftHandIKMode::Grip;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};