#pragma once

#include "CoreMinimal.h"
#include "MasterItemEnums.generated.h"

/**
 * Категории предметов (для PrimaryDataAsset / DT / UI)
 * В редакторе будет показываться DisplayName ("Weapons"), а не "ItemCat_Weapons".
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	ItemCat_None               UMETA(DisplayName="None"),

	ItemCat_Weapons            UMETA(DisplayName="Weapons"),
	ItemCat_Attachments        UMETA(DisplayName="Attachments"),
	ItemCat_HeroParts          UMETA(DisplayName="Hero Parts"),
	ItemCat_Armor              UMETA(DisplayName="Armor"),
	ItemCat_Helmet             UMETA(DisplayName="Helmet"),
	ItemCat_HelmetAttachments  UMETA(DisplayName="Helmet Attachments"),
	ItemCat_Backpack           UMETA(DisplayName="Backpack"),
	ItemCat_UsableItems        UMETA(DisplayName="UsableItems"),
	ItemCat_Ammo               UMETA(DisplayName="Ammo"),
	ItemCat_Items              UMETA(DisplayName="Items"),
	ItemCat_Placeable          UMETA(DisplayName="Placeable"),
};

UENUM(BlueprintType)
enum class EItemSubCategory : uint8
{
	ItemSubCat_None UMETA(DisplayName="None"),

	// ItemCat_Weapons
	ItemSubCat_Weapons_ASR     UMETA(DisplayName="ASR"),
	ItemSubCat_Weapons_SNP     UMETA(DisplayName="SNP"),
	ItemSubCat_Weapons_SMG     UMETA(DisplayName="SMG"),
	ItemSubCat_Weapons_SHTG    UMETA(DisplayName="SHTG"),
	ItemSubCat_Weapons_MG      UMETA(DisplayName="MG"),
	ItemSubCat_Weapons_HG      UMETA(DisplayName="HG"),
	ItemSubCat_Weapons_Knife   UMETA(DisplayName="Knife"),
	ItemSubCat_Weapons_RPG     UMETA(DisplayName="RPG"),
	ItemSubCat_Weapons_Grenade UMETA(DisplayName="Grenade"),

	// ItemCat_Attachments
	ItemSubCat_Attachments_Scope       UMETA(DisplayName="Scope"),
	ItemSubCat_Attachments_Silencer    UMETA(DisplayName="Silencer"),
	ItemSubCat_Attachments_Muzzle      UMETA(DisplayName="Muzzle"),
	ItemSubCat_Attachments_Magazine    UMETA(DisplayName="Magazine"),
	ItemSubCat_Attachments_Grip        UMETA(DisplayName="Grip"),
	ItemSubCat_Attachments_Laser       UMETA(DisplayName="Laser"),
	ItemSubCat_Attachments_FlashLights UMETA(DisplayName="FlashLights"),

	// ItemCat_HeroParts
	ItemSubCat_HeroParts_Head     UMETA(DisplayName="Head"),
	ItemSubCat_HeroParts_Body     UMETA(DisplayName="Body"),
	ItemSubCat_HeroParts_Hands    UMETA(DisplayName="Hands"),
	ItemSubCat_HeroParts_FPSHands UMETA(DisplayName="FPS Hands"),

	// ItemCat_HelmetAttachments
	ItemSubCat_HelmetAttachments_Visual1 UMETA(DisplayName="Visual 1"),
	ItemSubCat_HelmetAttachments_Visual2 UMETA(DisplayName="Visual 2"),

	// ItemCat_UsableItems
	ItemSubCat_UsableItems_Binocular  UMETA(DisplayName="Binocular"),
	ItemSubCat_UsableItems_PDA        UMETA(DisplayName="PDA"),
	ItemSubCat_UsableItems_Detector   UMETA(DisplayName="Detector"),
	ItemSubCat_UsableItems_FlashLight UMETA(DisplayName="FlashLight"),

	// ItemCat_Items
	ItemSubCat_Items_Artefacts   UMETA(DisplayName="Artefacts"),
	ItemSubCat_Items_Modules     UMETA(DisplayName="Modules"),
	ItemSubCat_Items_Trophy      UMETA(DisplayName="Trophy"),
	ItemSubCat_Items_Tools       UMETA(DisplayName="Tools"),
	ItemSubCat_Items_Components  UMETA(DisplayName="Components"),
	ItemSubCat_Items_Repair      UMETA(DisplayName="Repair Items"),
	ItemSubCat_Items_Recipe      UMETA(DisplayName="Recipe"),
	ItemSubCat_Items_Food        UMETA(DisplayName="Food"),
	ItemSubCat_Items_Water       UMETA(DisplayName="Water"),
	ItemSubCat_Items_Medicine    UMETA(DisplayName="Medicine"),
	ItemSubCat_Items_Upgrade     UMETA(DisplayName="Upgrade Items"),
};

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	AmmoType_None UMETA(DisplayName="None"),

	AmmoType_45ACP               UMETA(DisplayName=".45 ACP"),
	AmmoType_223                 UMETA(DisplayName=".223"),
	AmmoType_45ACP_Hydroshock    UMETA(DisplayName=".45 ACP Hydroshock"),

	AmmoType_12x70_Drob          UMETA(DisplayName="12x70 Дробь"),
	AmmoType_12x70_Drotik        UMETA(DisplayName="12x70 Дротик"),
	AmmoType_12x76_Zhekan        UMETA(DisplayName="12x76 Жекан"),

	AmmoType_545x39              UMETA(DisplayName="5.45x39"),
	AmmoType_545x39_BP           UMETA(DisplayName="5.45x39 БП"),

	AmmoType_556x45              UMETA(DisplayName="5.56x45"),
	AmmoType_556x45_AP           UMETA(DisplayName="5.56x45 AP"),

	AmmoType_57x28               UMETA(DisplayName="5.7x28"),
	AmmoType_57x28_AP            UMETA(DisplayName="5.7x28 AP"),

	AmmoType_762x54R_7H14        UMETA(DisplayName="7.62x54 R 7H14"),
	AmmoType_762x54_7H1          UMETA(DisplayName="7.62x54 7H1"),
	AmmoType_762x54_BP           UMETA(DisplayName="7.62x54 БП"),
	AmmoType_762x54_PP           UMETA(DisplayName="7.62x54 ПП"),

	AmmoType_9x18_PlusPPlus      UMETA(DisplayName="9x18 +P+"),
	AmmoType_9x18_PBP            UMETA(DisplayName="9x18 PBP"),

	AmmoType_9x19_FMJ            UMETA(DisplayName="9x19 FMJ"),
	AmmoType_9x19_JHP            UMETA(DisplayName="9x19 JHP"),

	AmmoType_9x39_PAB9           UMETA(DisplayName="9x39 ПАБ-9"),
	AmmoType_9x39_SP5            UMETA(DisplayName="9x39 СП-5"),
	AmmoType_9x39_SP6            UMETA(DisplayName="9x39 СП-6"),
};

// (опционально) фильтр вкладок/сортировки UI
UENUM(BlueprintType)
enum class EItemFilter : uint8
{
	Filter_None UMETA(DisplayName="None"),
	Filter_All  UMETA(DisplayName="All"),
};