// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Toolsets/UnrealMCPExtendedToolset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnrealMCPExtendedPIEInputRejectsUnregisteredKeyTest,
	"UnrealMCPExtended.Input.PIEInputRejectsUnregisteredKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnrealMCPExtendedPIEInputRejectsUnregisteredKeyTest::RunTest(const FString& Parameters)
{
	const FUnrealMCPExtendedPIEKeyEventResult Result =
		UUnrealMCPExtendedToolset::SendKeyEventToActivePIEViewport(
			TEXT("CODEX_Invalid_Input_Key"), true);
	TestFalse(TEXT("An unregistered key is rejected"), Result.bSuccess);
	TestFalse(TEXT("No event is reported as handled"), Result.bHandled);
	TestEqual(TEXT("The invalid-key error is explained"), Result.Error,
		FString(TEXT("Unreal does not recognize the requested input key.")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnrealMCPExtendedAssetRegistryQueriesExposeDiskReferencesTest,
	"UnrealMCPExtended.AssetRegistry.QueriesExposeDiskReferences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnrealMCPExtendedAssetRegistryQueriesExposeDiskReferencesTest::RunTest(const FString& Parameters)
{
	TArray<FString> ExcludedClassPaths;
	ExcludedClassPaths.Add(TEXT("/Script/ControlRigDeveloper.ControlRigBlueprint"));
	const FUnrealMCPExtendedAssetListResult FirstPage =
		UUnrealMCPExtendedToolset::ListAssetsByPath(TEXT("/Game"), true, true, ExcludedClassPaths, 0, 1);
	TestTrue(TEXT("The on-disk /Game inventory query succeeds"), FirstPage.bSuccess);
	TestTrue(TEXT("The inventory reports that it uses on-disk data"), FirstPage.bOnDiskOnly);
	TestEqual(TEXT("The requested class exclusion is returned"), FirstPage.ExcludedClassPaths.Num(), 1);
	TestTrue(TEXT("The FactoryGame /Game inventory is non-empty"), FirstPage.TotalCount > 1);
	TestEqual(TEXT("The first page respects its limit"), FirstPage.Assets.Num(), 1);
	TestTrue(TEXT("A one-entry page reports more available assets"), FirstPage.bHasMore);
	if (!FirstPage.bSuccess || FirstPage.Assets.IsEmpty() || FirstPage.TotalCount < 2)
	{
		return false;
	}

	const FUnrealMCPExtendedAssetListResult SecondPage =
		UUnrealMCPExtendedToolset::ListAssetsByPath(TEXT("/Game"), true, true, ExcludedClassPaths, 1, 1);
	TestTrue(TEXT("The second on-disk page succeeds"), SecondPage.bSuccess);
	TestEqual(TEXT("The second page contains its first stable entry"), SecondPage.Assets.Num(), 1);
	if (SecondPage.Assets.Num() == 1)
	{
		TestTrue(TEXT("Pagination advances to a different object"),
			FirstPage.Assets[0].ObjectPath != SecondPage.Assets[0].ObjectPath);
	}

	const FUnrealMCPExtendedAssetListResult InvalidPage =
		UUnrealMCPExtendedToolset::ListAssetsByPath(TEXT("/Game"), true, true, ExcludedClassPaths, -1, 1);
	TestFalse(TEXT("A negative page offset is rejected"), InvalidPage.bSuccess);
	TestTrue(TEXT("Invalid pagination explains the error"), !InvalidPage.Error.IsEmpty());

	const FUnrealMCPExtendedAssetReferencesResult References =
		UUnrealMCPExtendedToolset::QueryAssetReferences(
			TEXT("/Game/Blueprints/Interfaces/BPI_Character"), true, true, false, false,
			ExcludedClassPaths, 0, 500);
	TestTrue(TEXT("The BPI_Character package query succeeds"), References.bSuccess);
	TestTrue(TEXT("The package references are explicitly on-disk"), References.bOnDisk);
	TestEqual(TEXT("The reference query reports its class exclusion"), References.ExcludedClassPaths.Num(), 1);
	const bool bFindsFirstPersonReferencer = References.Referencers.ContainsByPredicate(
		[](const FUnrealMCPExtendedAssetReference& Reference)
		{
			return Reference.PackageName == TEXT("/Game/Blueprints/Character/BP_FirstPersonCharacter");
		});
	TestTrue(TEXT("The reverse inventory includes the known FirstPerson package reference"),
		bFindsFirstPersonReferencer);
	TArray<FString> ExcludedBlueprintClassPaths;
	ExcludedBlueprintClassPaths.Add(TEXT("/Script/Engine.Blueprint"));
	const FUnrealMCPExtendedAssetListResult ExcludedBlueprintAssets =
		UUnrealMCPExtendedToolset::ListAssetsByPath(
			TEXT("/Game/Blueprints/Interfaces"), true, true, ExcludedBlueprintClassPaths, 0, 500);
	TestTrue(TEXT("The interface-folder class-filter query succeeds"), ExcludedBlueprintAssets.bSuccess);
	TestEqual(TEXT("The recursive Blueprint class exclusion hides the interface assets"),
		ExcludedBlueprintAssets.TotalCount, 0);

	const FUnrealMCPExtendedAssetReferencesResult ExcludedBlueprintReferences =
		UUnrealMCPExtendedToolset::QueryAssetReferences(
			TEXT("/Game/Blueprints/Interfaces/BPI_Character"), true, true, false, false,
			ExcludedBlueprintClassPaths, 0, 500);
	TestTrue(TEXT("The excluded-reference query succeeds"), ExcludedBlueprintReferences.bSuccess);
	TestTrue(TEXT("The excluded-reference query uses on-disk data"), ExcludedBlueprintReferences.bOnDisk);
	TestEqual(TEXT("Blueprint referencers are suppressed from the result"),
		ExcludedBlueprintReferences.TotalReferencers, 0);
	TestTrue(TEXT("Suppressed class references are counted"),
		ExcludedBlueprintReferences.ExcludedReferenceCount > 0);

	const FUnrealMCPExtendedAssetReferencesResult MissingPackage =
		UUnrealMCPExtendedToolset::QueryAssetReferences(
			TEXT("/Game/__CODEX_AssetRegistryProbe_Missing"), true, true, false, false,
			TArray<FString>(), 0, 10);
	TestFalse(TEXT("A missing package query fails"), MissingPackage.bSuccess);
	TestFalse(TEXT("A failed package query is not reported as on-disk"), MissingPackage.bOnDisk);
	TestTrue(TEXT("The missing-package error is explained"), !MissingPackage.Error.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
