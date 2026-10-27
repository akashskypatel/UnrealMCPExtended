// Copyright Epic Games, Inc. All Rights Reserved.

#include "Toolsets/UnrealMCPExtendedToolset.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/PackageName.h"
#include "UnrealClient.h"

namespace
{
constexpr int32 MaxAssetRegistryPageSize = 500;

bool ValidatePage(const int32 Offset, const int32 Limit, FString& Error)
{
	if (Offset < 0 || Limit <= 0 || Limit > MaxAssetRegistryPageSize)
	{
		Error = FString::Printf(
			TEXT("Offset must be non-negative and Limit must be between 1 and %d."),
			MaxAssetRegistryPageSize);
		return false;
	}
	return true;
}

bool GatherReferenceDirection(
	IAssetRegistry& Registry,
	const FAssetIdentifier& AssetIdentifier,
	const bool bReferencers,
	const bool bIncludeHard,
	const bool bIncludeSoft,
	const bool bIncludeManagement,
	const bool bIncludeSearchableNames,
	TArray<FAssetDependency>& OutReferences)
{
	auto Query = [&Registry, &AssetIdentifier, bReferencers, &OutReferences](
		const UE::AssetRegistry::EDependencyCategory Category,
		const UE::AssetRegistry::FDependencyQuery& Flags)
	{
		return bReferencers
			? Registry.GetReferencers(AssetIdentifier, OutReferences, Category, Flags)
			: Registry.GetDependencies(AssetIdentifier, OutReferences, Category, Flags);
	};

	if (bIncludeHard || bIncludeSoft)
	{
		UE::AssetRegistry::FDependencyQuery PackageFlags;
		if (bIncludeHard != bIncludeSoft)
		{
			PackageFlags = UE::AssetRegistry::FDependencyQuery(
				bIncludeHard
					? UE::AssetRegistry::EDependencyQuery::Hard
					: UE::AssetRegistry::EDependencyQuery::NotHard);
		}
		if (!Query(UE::AssetRegistry::EDependencyCategory::Package, PackageFlags))
		{
			return false;
		}
	}
	if (bIncludeManagement)
	{
		const UE::AssetRegistry::FDependencyQuery ManageFlags;
		if (!Query(UE::AssetRegistry::EDependencyCategory::Manage, ManageFlags))
		{
			return false;
		}
	}
	if (bIncludeSearchableNames)
	{
		const UE::AssetRegistry::FDependencyQuery SearchableNameFlags;
		if (!Query(UE::AssetRegistry::EDependencyCategory::SearchableName, SearchableNameFlags))
		{
			return false;
		}
	}
	return true;
}

FString DependencyCategoryToString(const UE::AssetRegistry::EDependencyCategory Category)
{
	const uint8 Bits = static_cast<uint8>(Category);
	TArray<FString> Names;
	if ((Bits & static_cast<uint8>(UE::AssetRegistry::EDependencyCategory::Package)) != 0)
	{
		Names.Add(TEXT("Package"));
	}
	if ((Bits & static_cast<uint8>(UE::AssetRegistry::EDependencyCategory::Manage)) != 0)
	{
		Names.Add(TEXT("Manage"));
	}
	if ((Bits & static_cast<uint8>(UE::AssetRegistry::EDependencyCategory::SearchableName)) != 0)
	{
		Names.Add(TEXT("SearchableName"));
	}
	return FString::Join(Names, TEXT("|"));
}

FUnrealMCPExtendedAssetReference ToAssetReference(const FAssetDependency& Dependency)
{
	FUnrealMCPExtendedAssetReference Result;
	Result.Identifier = Dependency.AssetId.ToString();
	Result.PackageName = Dependency.AssetId.PackageName.IsNone()
		? FString() : Dependency.AssetId.PackageName.ToString();
	Result.ObjectName = Dependency.AssetId.ObjectName.IsNone()
		? FString() : Dependency.AssetId.ObjectName.ToString();
	Result.ValueName = Dependency.AssetId.ValueName.IsNone()
		? FString() : Dependency.AssetId.ValueName.ToString();
	Result.Category = DependencyCategoryToString(Dependency.Category);

	const uint8 Bits = static_cast<uint8>(Dependency.Properties);
	auto AddProperty = [&Result, Bits](const UE::AssetRegistry::EDependencyProperty Property, const TCHAR* Name)
	{
		if ((Bits & static_cast<uint8>(Property)) != 0)
		{
			Result.Properties.Add(Name);
		}
	};
	AddProperty(UE::AssetRegistry::EDependencyProperty::Hard, TEXT("Hard"));
	AddProperty(UE::AssetRegistry::EDependencyProperty::Game, TEXT("Game"));
	AddProperty(UE::AssetRegistry::EDependencyProperty::Build, TEXT("Build"));
	AddProperty(UE::AssetRegistry::EDependencyProperty::Direct, TEXT("Direct"));
	AddProperty(UE::AssetRegistry::EDependencyProperty::CookRule, TEXT("CookRule"));
	return Result;
}

void AddReferencePage(
	const TArray<FAssetDependency>& Source,
	const int32 Offset,
	const int32 Limit,
	TArray<FUnrealMCPExtendedAssetReference>& OutPage,
	bool& bOutHasMore)
{
	const int32 Start = FMath::Min(Offset, Source.Num());
	const int64 RequestedEnd = static_cast<int64>(Offset) + Limit;
	const int32 End = static_cast<int32>(FMath::Min<int64>(Source.Num(), RequestedEnd));
	OutPage.Reserve(End - Start);
	for (int32 Index = Start; Index < End; ++Index)
	{
		OutPage.Add(ToAssetReference(Source[Index]));
	}
	bOutHasMore = End < Source.Num();
}
}

FUnrealMCPExtendedPIEKeyEventResult UUnrealMCPExtendedToolset::SendKeyEventToActivePIEViewport(
	const FString& KeyName, const bool bPressed)
{
	FUnrealMCPExtendedPIEKeyEventResult Result;
	const FString NormalizedKeyName = KeyName.TrimStartAndEnd();
	const FKey Key(*NormalizedKeyName);
	Result.KeyName = NormalizedKeyName;
	Result.bPressed = bPressed;
	Result.EventName = bPressed ? TEXT("Pressed") : TEXT("Released");

	if (!IsInGameThread())
	{
		Result.Error = TEXT("PIE key events must be sent from the Unreal game thread.");
		return Result;
	}
	if (!Key.IsValid() || !EKeys::GetKeyDetails(Key).IsValid())
	{
		Result.Error = TEXT("Unreal does not recognize the requested input key.");
		return Result;
	}
	if (!GEditor)
	{
		Result.Error = TEXT("The Unreal Editor is not available.");
		return Result;
	}

	FViewport* PIEViewport = GEditor->GetPIEViewport();
	if (!PIEViewport)
	{
		Result.Error = TEXT("There is no active PIE viewport.");
		return Result;
	}

	FViewportClient* ViewportClient = PIEViewport->GetClient();
	if (!ViewportClient)
	{
		Result.Error = TEXT("The active PIE viewport has no input client.");
		return Result;
	}

	const FInputKeyEventArgs EventArgs(
		PIEViewport, FInputDeviceId::CreateFromInternalId(0), Key,
		bPressed ? IE_Pressed : IE_Released, 1.0f, false,
		FPlatformTime::Cycles64());
	Result.bHandled = ViewportClient->InputKey(EventArgs);
	Result.bSuccess = true;
	return Result;
}

FUnrealMCPExtendedAssetListResult UUnrealMCPExtendedToolset::ListAssetsByPath(
	const FString& PackagePath, const bool bRecursive, const bool bOnDiskOnly,
	const TArray<FString>& ExcludedClassPaths,
	const int32 Offset, const int32 Limit)
{
	FUnrealMCPExtendedAssetListResult Result;
	Result.PackagePath = PackagePath.TrimStartAndEnd();
	Result.bOnDiskOnly = bOnDiskOnly;
	Result.Offset = Offset;
	Result.Limit = Limit;
	Result.ExcludedClassPaths = ExcludedClassPaths;

	if (!IsInGameThread())
	{
		Result.Error = TEXT("Asset Registry queries must run on the Unreal game thread.");
		return Result;
	}
	if (!FPackageName::IsValidPath(Result.PackagePath))
	{
		Result.Error = TEXT("PackagePath must be a valid Unreal package path, such as /Game/Blueprints.");
		return Result;
	}
	if (!ValidatePage(Offset, Limit, Result.Error))
	{
		return Result;
	}

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*Result.PackagePath));
	Filter.bRecursivePaths = bRecursive;
	Filter.bIncludeOnlyOnDiskAssets = bOnDiskOnly;
	for (FString& ExcludedClassPath : Result.ExcludedClassPaths)
	{
		ExcludedClassPath = ExcludedClassPath.TrimStartAndEnd();
		const FTopLevelAssetPath ClassPath(ExcludedClassPath);
		if (!ClassPath.IsValid())
		{
			Result.Error = FString::Printf(TEXT("Excluded class path '%s' is invalid."), *ExcludedClassPath);
			return Result;
		}
		Filter.RecursiveClassPathsExclusionSet.Add(ClassPath);
	}
	Filter.bRecursiveClasses = !Filter.RecursiveClassPathsExclusionSet.IsEmpty();

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	if (!AssetRegistryModule.Get().GetAssets(Filter, Assets))
	{
		Result.Error = TEXT("The Asset Registry could not query the requested package path.");
		return Result;
	}

	Assets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		const int32 PackageOrder = Left.PackageName.ToString().Compare(
			Right.PackageName.ToString(), ESearchCase::CaseSensitive);
		if (PackageOrder != 0)
		{
			return PackageOrder < 0;
		}

		const int32 AssetNameOrder = Left.AssetName.ToString().Compare(
			Right.AssetName.ToString(), ESearchCase::CaseSensitive);
		if (AssetNameOrder != 0)
		{
			return AssetNameOrder < 0;
		}

		return Left.GetSoftObjectPath().ToString().Compare(
			Right.GetSoftObjectPath().ToString(), ESearchCase::CaseSensitive) < 0;
	});

	Result.TotalCount = Assets.Num();
	const int32 Start = FMath::Min(Offset, Assets.Num());
	const int64 RequestedEnd = static_cast<int64>(Offset) + Limit;
	const int32 End = static_cast<int32>(FMath::Min<int64>(Assets.Num(), RequestedEnd));
	Result.Assets.Reserve(End - Start);
	for (int32 Index = Start; Index < End; ++Index)
	{
		const FAssetData& Asset = Assets[Index];
		FUnrealMCPExtendedAssetListEntry& Entry = Result.Assets.AddDefaulted_GetRef();
		Entry.PackageName = Asset.PackageName.ToString();
		Entry.PackagePath = Asset.PackagePath.ToString();
		Entry.AssetName = Asset.AssetName.ToString();
		Entry.AssetClassPath = Asset.AssetClassPath.ToString();
		Entry.ObjectPath = Asset.GetSoftObjectPath().ToString();
	}
	Result.bHasMore = End < Assets.Num();
	Result.bSuccess = true;
	return Result;
}

FUnrealMCPExtendedAssetReferencesResult UUnrealMCPExtendedToolset::QueryAssetReferences(
	const FString& PackagePath, const bool bIncludeHard, const bool bIncludeSoft,
	const bool bIncludeManagement, const bool bIncludeSearchableNames,
	const TArray<FString>& ExcludedClassPaths,
	const int32 Offset, const int32 Limit)
{
	FUnrealMCPExtendedAssetReferencesResult Result;
	Result.PackagePath = PackagePath.TrimStartAndEnd();
	Result.ExcludedClassPaths = ExcludedClassPaths;
	Result.Offset = Offset;
	Result.Limit = Limit;

	if (!IsInGameThread())
	{
		Result.Error = TEXT("Asset Registry queries must run on the Unreal game thread.");
		return Result;
	}
	if (!FPackageName::IsValidLongPackageName(Result.PackagePath))
	{
		Result.Error = TEXT("PackagePath must be a long package name without an object suffix.");
		return Result;
	}
	if (!ValidatePage(Offset, Limit, Result.Error))
	{
		return Result;
	}

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	TArray<FTopLevelAssetPath> ExcludedBaseClassPaths;
	TSet<FTopLevelAssetPath> ExcludedClassPathSet;
	for (FString& ExcludedClassPath : Result.ExcludedClassPaths)
	{
		ExcludedClassPath = ExcludedClassPath.TrimStartAndEnd();
		const FTopLevelAssetPath ClassPath(ExcludedClassPath);
		if (!ClassPath.IsValid())
		{
			Result.Error = FString::Printf(TEXT("Excluded class path '%s' is invalid."), *ExcludedClassPath);
			return Result;
		}
		ExcludedBaseClassPaths.Add(ClassPath);
		ExcludedClassPathSet.Add(ClassPath);
	}
	if (!ExcludedBaseClassPaths.IsEmpty())
	{
		const TSet<FTopLevelAssetPath> NoExcludedClassPaths;
		TSet<FTopLevelAssetPath> DerivedClassPaths;
		AssetRegistry.GetDerivedClassNames(ExcludedBaseClassPaths, NoExcludedClassPaths, DerivedClassPaths);
		ExcludedClassPathSet.Append(DerivedClassPaths);
	}

	const FAssetIdentifier AssetIdentifier(FName(*Result.PackagePath));
	TArray<FAssetDependency> Dependencies;
	TArray<FAssetDependency> Referencers;
	if (!GatherReferenceDirection(
		AssetRegistry, AssetIdentifier, false,
		bIncludeHard, bIncludeSoft, bIncludeManagement, bIncludeSearchableNames,
		Dependencies))
	{
		Result.Error = TEXT("The Asset Registry could not query dependencies for the requested package.");
		return Result;
	}
	if (!GatherReferenceDirection(
		AssetRegistry, AssetIdentifier, true,
		bIncludeHard, bIncludeSoft, bIncludeManagement, bIncludeSearchableNames,
		Referencers))
	{
		Result.Error = TEXT("The Asset Registry could not query referencers for the requested package.");
		return Result;
	}

	TMap<FName, int8> PackageClassFilterCache;
	auto SuppressExcludedOrUnresolved = [
		&AssetRegistry, &ExcludedClassPathSet, &PackageClassFilterCache, &Result](const FAssetDependency& Reference)
	{
		const FName PackageName = Reference.AssetId.PackageName;
		if (PackageName.IsNone())
		{
			++Result.UnresolvedIdentifierCount;
			return true;
		}

		const FString PackageNameString = PackageName.ToString();
		if (PackageNameString.StartsWith(TEXT("/Script/"), ESearchCase::IgnoreCase))
		{
			return false;
		}

		int8* FilterResult = PackageClassFilterCache.Find(PackageName);
		if (!FilterResult)
		{
			TArray<FAssetData> PackageAssets;
			const bool bFoundOnDiskAssets = AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets, true);
			int8 Decision = (bFoundOnDiskAssets && !PackageAssets.IsEmpty()) ? 0 : -1;
			if (Decision == 0)
			{
				for (const FAssetData& PackageAsset : PackageAssets)
				{
					if (ExcludedClassPathSet.Contains(PackageAsset.AssetClassPath))
					{
						Decision = 1;
						break;
					}
				}
			}
			FilterResult = &PackageClassFilterCache.Add(PackageName, Decision);
		}

		if (*FilterResult < 0)
		{
			++Result.UnresolvedIdentifierCount;
			return true;
		}
		if (*FilterResult > 0)
		{
			++Result.ExcludedReferenceCount;
			return true;
		}
		return false;
	};
	Dependencies.RemoveAll(SuppressExcludedOrUnresolved);
	Referencers.RemoveAll(SuppressExcludedOrUnresolved);

	auto SortReferences = [](TArray<FAssetDependency>& Values)
	{
		Values.Sort([](const FAssetDependency& Left, const FAssetDependency& Right)
		{
			return Left.LexicalLess(Right);
		});
	};
	SortReferences(Dependencies);
	SortReferences(Referencers);

	Result.TotalDependencies = Dependencies.Num();
	Result.TotalReferencers = Referencers.Num();
	AddReferencePage(Dependencies, Offset, Limit, Result.Dependencies, Result.bHasMoreDependencies);
	AddReferencePage(Referencers, Offset, Limit, Result.Referencers, Result.bHasMoreReferencers);
	Result.bOnDisk = true;
	Result.bSuccess = true;
	return Result;
}
