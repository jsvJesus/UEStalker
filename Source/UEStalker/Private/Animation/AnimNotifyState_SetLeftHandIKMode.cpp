#include "Animation/AnimNotifyState_SetLeftHandIKMode.h"
#include "Components/SkeletalMeshComponent.h"

static AMasterCharacter* GetMasterCharacterFromMesh(USkeletalMeshComponent* MeshComp)
{
	if (!IsValid(MeshComp))
	{
		return nullptr;
	}
	return Cast<AMasterCharacter>(MeshComp->GetOwner());
}

void UAnimNotifyState_SetLeftHandIKMode::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AMasterCharacter* C = GetMasterCharacterFromMesh(MeshComp))
	{
		C->SetLeftHandIKMode(BeginMode);
	}
}

void UAnimNotifyState_SetLeftHandIKMode::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AMasterCharacter* C = GetMasterCharacterFromMesh(MeshComp))
	{
		C->SetLeftHandIKMode(EndMode);
	}
}
