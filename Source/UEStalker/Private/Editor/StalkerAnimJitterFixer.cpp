#if WITH_EDITOR

#include "Editor/StalkerAnimJitterFixer.h"

#include "Editor.h"
#include "Factories/Factory.h"

#include "AssetRegistry/AssetRegistryModule.h"

#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimBoneCompressionSettings.h"
#include "Animation/AnimCurveCompressionSettings.h"
#include "Animation/AnimCurveCompressionCodec_UniformIndexable.h"
#include "Animation/AnimCompress_BitwiseCompressOnly.h"
#include "Animation/AnimCompress.h"

#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogStalkerAnimJitterFix, Log, All);

namespace StalkerAnimJitterFix
{
	static const TCHAR* BoneSettingsPackage = TEXT("/Game/_UEStalker/Animation/ABCS_StalkerLossless");
	static const TCHAR* CurveSettingsPackage = TEXT("/Game/_UEStalker/Animation/ACCS_StalkerCurves");

	static TWeakObjectPtr<UAnimBoneCompressionSettings> GBoneSettings;
	static TWeakObjectPtr<UAnimCurveCompressionSettings> GCurveSettings;

	static FDelegateHandle PostImportHandle;

	static FString MakeObjectPath(const TCHAR* PackageName)
	{
		const FString Pkg = PackageName;
		const FString AssetName = FPackageName::GetLongPackageAssetName(Pkg);
		return FString::Printf(TEXT("%s.%s"), *Pkg, *AssetName);
	}

	static UObject* LoadObjectByPath(const FString& ObjectPath)
	{
		return LoadObject<UObject>(nullptr, *ObjectPath);
	}

	static bool SaveAssetPackage(UPackage* Package, UObject* AssetObject)
	{
		if (!Package || !AssetObject)
		{
			return false;
		}

		const FString PackageName = Package->GetName();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackageName,
			FPackageName::GetAssetPackageExtension()
		);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;

		return UPackage::SavePackage(Package, AssetObject, *Filename, SaveArgs);
	}

	static UAnimBoneCompressionSettings* EnsureBoneSettings()
	{
		if (GBoneSettings.IsValid())
		{
			return GBoneSettings.Get();
		}

		const FString ObjPath = MakeObjectPath(BoneSettingsPackage);
		if (UAnimBoneCompressionSettings* Existing = Cast<UAnimBoneCompressionSettings>(LoadObjectByPath(ObjPath)))
		{
			GBoneSettings = Existing;
			return Existing;
		}

		const FString PkgName = BoneSettingsPackage;
		UPackage* Package = CreatePackage(*PkgName);
		Package->FullyLoad();

		const FString AssetName = FPackageName::GetLongPackageAssetName(PkgName);

		UAnimBoneCompressionSettings* Settings =
			NewObject<UAnimBoneCompressionSettings>(Package, *AssetName, RF_Public | RF_Standalone);

		// максимально «без потерь»
		Settings->ErrorThreshold = 0.0f;
		Settings->bForceBelowThreshold = true;
		Settings->Codecs.Reset();

		UAnimCompress_BitwiseCompressOnly* Codec =
			NewObject<UAnimCompress_BitwiseCompressOnly>(Settings, TEXT("Codec_BitwiseLossless"), RF_Transactional);

		// ACF_None = вообще без битовой компрессии (сырые float'ы), зато убирает дрожь на 100%
		Codec->TranslationCompressionFormat = ACF_None;
		Codec->RotationCompressionFormat    = ACF_None;
		Codec->ScaleCompressionFormat       = ACF_None;

		Settings->Codecs.Add(Codec);

		FAssetRegistryModule::AssetCreated(Settings);
		Package->MarkPackageDirty();
		SaveAssetPackage(Package, Settings);

		GBoneSettings = Settings;
		UE_LOG(LogStalkerAnimJitterFix, Display, TEXT("Created bone compression settings: %s"), *ObjPath);

		return Settings;
	}

	static UAnimCurveCompressionSettings* EnsureCurveSettings()
	{
		if (GCurveSettings.IsValid())
		{
			return GCurveSettings.Get();
		}

		const FString ObjPath = MakeObjectPath(CurveSettingsPackage);
		if (UAnimCurveCompressionSettings* Existing = Cast<UAnimCurveCompressionSettings>(LoadObjectByPath(ObjPath)))
		{
			GCurveSettings = Existing;
			return Existing;
		}

		const FString PkgName = CurveSettingsPackage;
		UPackage* Package = CreatePackage(*PkgName);
		Package->FullyLoad();

		const FString AssetName = FPackageName::GetLongPackageAssetName(PkgName);

		UAnimCurveCompressionSettings* Settings =
			NewObject<UAnimCurveCompressionSettings>(Package, *AssetName, RF_Public | RF_Standalone);

		UAnimCurveCompressionCodec_UniformIndexable* Codec =
			NewObject<UAnimCurveCompressionCodec_UniformIndexable>(Settings, TEXT("Codec_UniformIndexable"), RF_Transactional);

		Settings->Codec = Codec;

		FAssetRegistryModule::AssetCreated(Settings);
		Package->MarkPackageDirty();
		SaveAssetPackage(Package, Settings);

		GCurveSettings = Settings;
		UE_LOG(LogStalkerAnimJitterFix, Display, TEXT("Created curve compression settings: %s"), *ObjPath);

		return Settings;
	}

	static void ApplyToAnimSequence(UAnimSequence* Anim)
	{
		if (!Anim)
		{
			return;
		}

		UAnimBoneCompressionSettings* Bone = EnsureBoneSettings();
		UAnimCurveCompressionSettings* Curve = EnsureCurveSettings();

		Anim->Modify();

		// главное: ставим наши settings
		Anim->BoneCompressionSettings = Bone;
		Anim->CurveCompressionSettings = Curve;

		// отключаем frame stripping (он часто убивает “высокочастотные” движения и дает микродрожь)
		Anim->bAllowFrameStripping = false;

		// чтобы проектные/платформенные overrides не перетирали
		Anim->bDoNotOverrideCompression = true;

		Anim->CompressionErrorThresholdScale = 1.0f;

		// пересобираем derived/compressed данные
		Anim->CacheDerivedDataForCurrentPlatform();

		Anim->MarkPackageDirty();
		Anim->PostEditChange();
	}

	static void OnAssetPostImport(UFactory* /*Factory*/, UObject* Object)
	{
		if (UAnimSequence* Anim = Cast<UAnimSequence>(Object))
		{
			ApplyToAnimSequence(Anim);
			UE_LOG(LogStalkerAnimJitterFix, Display, TEXT("Auto-fixed imported AnimSequence: %s"), *Anim->GetPathName());
		}
	}

	static void FixAllAnimSequences()
	{
		EnsureBoneSettings();
		EnsureCurveSettings();

		FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		FARFilter Filter;
		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Add(FName("/Game"));
		Filter.ClassPaths.Add(UAnimSequence::StaticClass()->GetClassPathName());

		TArray<FAssetData> Assets;
		ARM.Get().GetAssets(Filter, Assets);

		int32 Fixed = 0;
		for (const FAssetData& AD : Assets)
		{
			UAnimSequence* Anim = Cast<UAnimSequence>(AD.GetAsset());
			if (!Anim)
			{
				continue;
			}

			ApplyToAnimSequence(Anim);
			++Fixed;
		}

		UE_LOG(LogStalkerAnimJitterFix, Display, TEXT("Stalker.FixAnimJitter done. Fixed: %d. (Save All чтобы записать на диск)"), Fixed);
	}

	static FAutoConsoleCommand CmdFixAll(
		TEXT("Stalker.FixAnimJitter"),
		TEXT("Apply lossless compression settings to all AnimSequence assets in /Game and disable frame stripping. Then Save All."),
		FConsoleCommandDelegate::CreateStatic(&FixAllAnimSequences)
	);
}

void FStalkerAnimJitterFixer::Startup()
{
	using namespace StalkerAnimJitterFix;

	EnsureBoneSettings();
	EnsureCurveSettings();

	if (!PostImportHandle.IsValid())
	{
		PostImportHandle = FEditorDelegates::OnAssetPostImport.AddStatic(&OnAssetPostImport);
	}

	UE_LOG(LogStalkerAnimJitterFix, Display, TEXT("StalkerAnimJitterFixer started. Console: Stalker.FixAnimJitter"));
}

void FStalkerAnimJitterFixer::Shutdown()
{
	using namespace StalkerAnimJitterFix;

	if (PostImportHandle.IsValid())
	{
		FEditorDelegates::OnAssetPostImport.Remove(PostImportHandle);
		PostImportHandle.Reset();
	}
	GBoneSettings.Reset();
	GCurveSettings.Reset();
}

#endif // WITH_EDITOR