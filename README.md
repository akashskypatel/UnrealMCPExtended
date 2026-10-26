# Unreal MCP Extended

Editor tools for operations outside Blueprint tooling and the built-in Unreal MCP and Python commandlet APIs.

## PIE viewport input

`UnrealMCPExtended.UnrealMCPExtendedToolset.SendKeyEventToActivePIEViewport` sends one registered key press or release through the active in-process PIE viewport. Call it once with `bPressed=true` and once with `bPressed=false` to complete a key press. It requires an active PIE viewport and does not send desktop or operating-system input.
