// Copyright Epic Games, Inc. All Rights Reserved.

#include "Toolsets/UnrealMCPExtendedToolset.h"

#include "Editor.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"

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
