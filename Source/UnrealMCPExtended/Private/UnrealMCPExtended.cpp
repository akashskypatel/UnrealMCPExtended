// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealMCPExtended.h"
#include "ToolsetRegistry/UToolsetRegistry.h"
#include "Toolsets/UnrealMCPExtendedToolset.h"

#define LOCTEXT_NAMESPACE "FUnrealMCPExtendedModule"

void FUnrealMCPExtendedModule::StartupModule()
{
	UToolsetRegistry::RegisterToolsetClass(UUnrealMCPExtendedToolset::StaticClass());
}

void FUnrealMCPExtendedModule::ShutdownModule()
{
	UToolsetRegistry::UnregisterToolsetClass(UUnrealMCPExtendedToolset::StaticClass());
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealMCPExtendedModule, UnrealMCPExtended)
