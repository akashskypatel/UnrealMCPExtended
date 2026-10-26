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

#endif // WITH_DEV_AUTOMATION_TESTS
