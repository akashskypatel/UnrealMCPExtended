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
};
