
# Building

## As part of the Engine

Usually all it requires is to include the Plugin in an Unreal Engine Project or the Unreal Engine itself.
To do this, put your clone of the Plugin into either the Project Plugins folder or the Engine Plugins folder.

## Standalone

```powershell
RunUAT.bat BuildPlugin "-Plugin=Plugins\Tentacle\Tentacle.uplugin" "-package=$env:TEMP\Tentacle-PluginCompile"
```

Add `-StrictIncludes` to find compilation edge cases with unity builds.
Add `-TargetPlatforms=Win64+Linux`