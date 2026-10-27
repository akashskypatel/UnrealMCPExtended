// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "UnrealMCPExtendedToolset.generated.h"

USTRUCT(BlueprintType)
struct FUnrealMCPExtendedPIEKeyEventResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	bool bHandled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	bool bPressed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString KeyName;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString EventName;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString Error;
};

USTRUCT(BlueprintType)
struct FUnrealMCPExtendedAssetListEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString PackageName;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString PackagePath;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString AssetName;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString AssetClassPath;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString ObjectPath;
};

USTRUCT(BlueprintType)
struct FUnrealMCPExtendedAssetListResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	bool bOnDiskOnly = true;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString PackagePath;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	TArray<FString> ExcludedClassPaths;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	int32 TotalCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	int32 Offset = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	int32 Limit = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	bool bHasMore = false;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	TArray<FUnrealMCPExtendedAssetListEntry> Assets;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString Error;
};

USTRUCT(BlueprintType)
struct FUnrealMCPExtendedAssetReference
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString Identifier;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString PackageName;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString ObjectName;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString ValueName;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString Category;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	TArray<FString> Properties;
};

USTRUCT(BlueprintType)
struct FUnrealMCPExtendedAssetReferencesResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	bool bOnDisk = false;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString PackagePath;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	TArray<FString> ExcludedClassPaths;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	int32 TotalDependencies = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	int32 TotalReferencers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	int32 ExcludedReferenceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	int32 UnresolvedIdentifierCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	int32 Offset = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	int32 Limit = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	bool bHasMoreDependencies = false;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	bool bHasMoreReferencers = false;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	TArray<FUnrealMCPExtendedAssetReference> Dependencies;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	TArray<FUnrealMCPExtendedAssetReference> Referencers;

	UPROPERTY(BlueprintReadOnly, Category = "Unreal MCP Extended")
	FString Error;
};

/** MCP tools for editor operations that do not belong to Blueprint tooling. */
UCLASS(BlueprintType, Hidden)
class UUnrealMCPExtendedToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/** Sends one registered key press or release through the active PIE viewport input client. */
	UFUNCTION(Category = "Unreal MCP Extended", meta = (AICallable))
	static FUnrealMCPExtendedPIEKeyEventResult SendKeyEventToActivePIEViewport(
		const FString& KeyName, bool bPressed);

	/** Lists stable-sorted Asset Registry entries for a folder, with bounded pagination and class exclusions. */
	UFUNCTION(Category = "Unreal MCP Extended", meta = (AICallable))
	static FUnrealMCPExtendedAssetListResult ListAssetsByPath(
		const FString& PackagePath, bool bRecursive, bool bOnDiskOnly,
		const TArray<FString>& ExcludedClassPaths, int32 Offset, int32 Limit);

	/** Returns on-disk package dependencies and reverse referencers, including their registry flags. */
	UFUNCTION(Category = "Unreal MCP Extended", meta = (AICallable))
	static FUnrealMCPExtendedAssetReferencesResult QueryAssetReferences(
		const FString& PackagePath, bool bIncludeHard, bool bIncludeSoft,
		bool bIncludeManagement, bool bIncludeSearchableNames,
		const TArray<FString>& ExcludedClassPaths, int32 Offset, int32 Limit);
};
