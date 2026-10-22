# Project Agent Instructions

## Unreal MCP

- The active Unreal project is `LanRan/LanRan.uproject`.
- Monolith is the editor MCP bridge. Start with `monolith_status` before making any editor changes.
- If the editor is unavailable, ask for it to be started or run `LanRan/OpenLanRan.cmd` when startup is clearly in scope.
- Use `monolith_guide` and `monolith_discover` to inspect the supported actions, then `describe_query` for an action schema before calling it.
- Prefer read-only inspection first. Never overwrite or delete assets without confirming the exact asset paths and the requested outcome.
- The installed `LanRan/Plugins/Monolith` directory is downloaded locally and ignored by Git. Do not commit its binaries or generated `Saved` content.
- After installing or upgrading Monolith, restart Unreal Editor so the embedded server on port 9316 can reload.
