Debug menu, available at https://www.nexusmods.com/skyrimspecialedition/mods/136456

Feel free to use any code.

## Fork: Skyrim 1.7.x

This fork adds support for Skyrim Special Edition 1.7.x (checked against 1.7.99 and 1.7.104) next to 1.6.1170.

- Two hooks moved in 1.7 (the tree-manager cull call and the free-camera position call), so their offsets depend on the runtime now (`Hooks::IsRuntime17()` in `src/hooks.h`).
- `ng.cmake` builds `DebugMenu.dll` on alandtse's CommonLibSSE-NG for Skyrim 1.6.1170 and 1.7.x. It is included from a parent CMake project that defines `ng_plugin()` and provides vcpkg with `polyhook2`, `detours`, `glm`, `spdlog`, `simpleini`, `directxtk` and `nlohmann-json`.
- Skyrim VR and 1.6.1130 and older keep using the original build from `CMakeLists.txt`.
- The release zip contains only the DLL. Install the DebugMenu mod from Nexus (https://www.nexusmods.com/skyrimspecialedition/mods/136456) first and let the DLL overwrite its own.
