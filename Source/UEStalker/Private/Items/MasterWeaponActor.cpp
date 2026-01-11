#include "Items/MasterWeaponActor.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Items/ItemObject.h"

AMasterWeaponActor::AMasterWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->CastShadow = false;

	AttachmentsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AttachmentsRoot"));
	AttachmentsRoot->SetupAttachment(WeaponMesh);

	auto MakeAttach = [&](const TCHAR* Name) -> USkeletalMeshComponent*
	{
		USkeletalMeshComponent* C = CreateDefaultSubobject<USkeletalMeshComponent>(Name);
		C->SetupAttachment(AttachmentsRoot);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		C->CastShadow = false;
		return C;
	};

	MagMesh        = MakeAttach(TEXT("MagMesh"));
	MuzzleMesh     = MakeAttach(TEXT("MuzzleMesh"));
	ScopeMesh      = MakeAttach(TEXT("ScopeMesh"));
	GripMesh       = MakeAttach(TEXT("GripMesh"));
	LaserMesh      = MakeAttach(TEXT("LaserMesh"));
	FlashLightMesh = MakeAttach(TEXT("FlashLightMesh"));
}

void AMasterWeaponActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (WeaponMesh)
	{
		WeaponMesh->SetSkeletalMesh(DefaultWeapon);
	}

	if (MagMesh)        MagMesh->SetSkeletalMesh(DefaultMag);
	if (MuzzleMesh)     MuzzleMesh->SetSkeletalMesh(DefaultMuzzle);
	if (ScopeMesh)      ScopeMesh->SetSkeletalMesh(DefaultScope);
	if (GripMesh)       GripMesh->SetSkeletalMesh(DefaultGrip);
	if (LaserMesh)      LaserMesh->SetSkeletalMesh(DefaultLaser);
	if (FlashLightMesh) FlashLightMesh->SetSkeletalMesh(DefaultFlashLight);

	RefreshAttachmentSockets();
}

void AMasterWeaponActor::GetAimData(FWeaponAimSettings& OutSettings, UAnimSequence*& OutAimAnim,
	USkeletalMeshComponent*& OutWeaponMesh) const
{
	if (IsValid(AimData))
	{
		OutSettings = AimData->Settings;
		OutAimAnim  = AimData->AimAnim1P;
	}
	else
	{
		OutSettings = AimSettingsOverride;
		OutAimAnim  = AimAnimOverride;
	}

	// Ищем первый SkeletalMeshComponent внутри Actor'а оружия
	OutWeaponMesh = FindComponentByClass<USkeletalMeshComponent>();
}

void AMasterWeaponActor::GetReloadAnim(UAnimSequence*& OutReloadAnim) const
{
	OutReloadAnim = ReloadAnimOverride;
}

void AMasterWeaponActor::SetWeaponItemObject(UItemObject* InWeaponItemObject)
{
	WeaponItemObject = InWeaponItemObject;
	RefreshFromItemObject();
}

int32 AMasterWeaponActor::GetAmmoInInsertedMag() const
{
	if (IsValid(WeaponItemObject) && IsValid(WeaponItemObject->InsertedMagazine))
	{
		return FMath::Max(0, WeaponItemObject->InsertedMagazine->MagazineAmmoCount);
	}
	return 0;
}

int32 AMasterWeaponActor::GetInsertedMagCapacity() const
{
	if (IsValid(WeaponItemObject) && IsValid(WeaponItemObject->InsertedMagazine))
	{
		return FMath::Max(0, WeaponItemObject->InsertedMagazine->GetMagazineCapacity());
	}
	return 0;
}

void AMasterWeaponActor::RefreshFromItemObject()
{
	if (!MagMesh)
	{
		return;
	}

	const bool bHasMag = IsValid(WeaponItemObject) && IsValid(WeaponItemObject->InsertedMagazine);

	// По умолчанию MagMesh выставлен как DefaultMag в OnConstruction.
	// Здесь только включаем/выключаем видимость.
	MagMesh->SetVisibility(bHasMag, true);
	MagMesh->SetHiddenInGame(!bHasMag, true);
}

void AMasterWeaponActor::SetWeaponMesh(USkeletalMesh* NewMesh)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetSkeletalMesh(NewMesh);
		RefreshAttachmentSockets();
	}
}

void AMasterWeaponActor::SetAttachmentMesh(EWeaponAttachmentSlot Slot, USkeletalMesh* NewMesh)
{
	USkeletalMeshComponent* Target = nullptr;

	switch (Slot)
	{
	case EWeaponAttachmentSlot::Mag:        Target = MagMesh; break;
	case EWeaponAttachmentSlot::Muzzle:     Target = MuzzleMesh; break;
	case EWeaponAttachmentSlot::Scope:      Target = ScopeMesh; break;
	case EWeaponAttachmentSlot::Grip:       Target = GripMesh; break;
	case EWeaponAttachmentSlot::Laser:      Target = LaserMesh; break;
	case EWeaponAttachmentSlot::FlashLight: Target = FlashLightMesh; break;
	default: break;
	}

	if (Target)
	{
		Target->SetSkeletalMesh(NewMesh);
		RefreshAttachmentSockets();
	}
}

void AMasterWeaponActor::RefreshAttachmentSockets()
{
	if (!WeaponMesh) return;

	const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);

	if (MagMesh)        MagMesh->AttachToComponent(WeaponMesh, Rules, MagSocket);
	if (MuzzleMesh)     MuzzleMesh->AttachToComponent(WeaponMesh, Rules, MuzzleSocket);
	if (ScopeMesh)      ScopeMesh->AttachToComponent(WeaponMesh, Rules, ScopeSocket);
	if (GripMesh)       GripMesh->AttachToComponent(WeaponMesh, Rules, GripSocket);
	if (LaserMesh)      LaserMesh->AttachToComponent(WeaponMesh, Rules, LaserSocket);
	if (FlashLightMesh) FlashLightMesh->AttachToComponent(WeaponMesh, Rules, FlashLightSocket);
}