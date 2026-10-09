# Tripo API Unreal

Tripo API Unreal integrates Tripo API v3 text-to-model and image-to-model
generation directly into Unreal Engine for 3D content creation and game
development.

## Features

- Text-to-3D model generation from prompts
- Image-to-3D model generation from PNG and JPEG images
- Model selection for v3.1, v3.0, v2.5, and P1
- Native Unreal Editor window with a task queue and generation settings
- FBX conversion, download, and import into the current project
- Runtime Blueprint function library for generating and downloading models
- API key management and available/frozen credit display

## Prerequisites

- Unreal Engine 5.x
- A valid Tripo API key beginning with `tsk_`
- A stable internet connection
- A C++ project, or a Blueprint project with an empty C++ class added so Unreal
  can compile the plugin

## Install

Follow these steps to install **Tripo API Unreal** into an Unreal project.

### Install the Plugin Package

1. Download `Tripo_API_Unreal-V<version>.zip` from the matching GitHub Release
   and extract it.
2. Copy the extracted `Tripo3D` folder to `<Project>/Plugins/Tripo3D` so the
   manifest is at `<Project>/Plugins/Tripo3D/Tripo3D.uplugin`.
3. Open the project with the target Unreal Engine version. When prompted, allow
   Unreal to compile the plugin from source.
4. Open **Edit > Plugins**, search for **Tripo API Unreal**, enable it if
   necessary, and restart the editor when prompted.

> The ZIP is a source package. Do not copy `Binaries` or `Intermediate` between
> Unreal Engine versions; each target engine compiles the plugin locally.

### Install From Source

1. Clone this repository into `<Project>/Plugins/Tripo3D`.
2. Open the project with the target Unreal Engine version and allow it to build
   the plugin.
3. Enable **Tripo API Unreal** in **Edit > Plugins** if it is not enabled.

## Usage Guide - Editor

### Open the Plugin

Open the editor window from **Tools > Tripo API**.

### Set Up Your API Key

1. Enter an API key beginning with `tsk_` in the plugin window.
2. Confirm the key to enable generation and balance requests.

> **Credential storage:** The editor stores the API key in the project editor
> `.ini` as plain local preference data. It is not encrypted. Use a scoped key
> when available, and remove the stored key before sharing the project or
> machine.

### Generate a 3D Model Using Text

1. Select the **Text** tab and enter a prompt.
2. Select a model and configure optional generation settings.
3. Click **Generate**. The task is polled, converted to FBX when needed,
   downloaded, and imported into the project.

### Generate a 3D Model Using Image

1. Select the **Image** tab and choose a `.png`, `.jpg`, or `.jpeg` image.
2. Select a model and configure optional generation settings.
3. Click **Generate** to upload the image and create the model.

### Track Multiple Generation Tasks

- Generation tasks are submitted without a local concurrency cap. Service-side
  limits are reported on the rejected task row.
- Each task keeps its prompt or image, task ID, stage, and progress visible.
- Completed and failed tasks remain in the queue until **Clear Finished** is
  selected and do not consume an active-task slot.
- Failed tasks report the server error in the task row and Unreal Output Log.
- Ordinary task lookup failures back off and fail only after the third
  consecutive failure. HTTP 429 and API backpressure responses wait for
  `Retry-After` when available and do not mark a healthy task failed.
- Polling intervals scale with the active queue to keep aggregate task lookup
  traffic near five requests per second.
- The queue is session-only and starts empty when Unreal is restarted.
- Available and frozen credits refresh while tasks are active and after a task
  reaches a terminal state.

### Adjust Model Versions

- **v3.1-20260211:** Default model; supports adaptive face limit, texture, PBR,
  texture quality, auto size, and triangle/quad topology.
- **v3.0-20250812:** Supports the same exposed settings as v3.1.
- **v2.5-20250123:** Supports face limit, texture, PBR, and triangle/quad
  topology.
- **P1-20260311:** Supports a 500-20,000 face-limit range, texture, and PBR.

All models use a minimum face limit of 500. Default maximum values are 2,000,000
for v3 triangles, 500,000 for v2.5 triangles, 150,000 for v3/v2.5 quads, and
20,000 for P1.

## Usage Guide - Runtime

The `TripoRuntime` module provides Blueprint nodes for API-key handling, account
balance, text-to-model, image upload, image-to-model, task polling, model
conversion, and model download.

For runtime display of downloaded GLB models, use a compatible runtime loader
such as [glTFRuntime](https://github.com/rdeioris/glTFRuntime). This plugin does
not include a runtime model renderer.

![Runtime Blueprint example](BPDisplay.jpg)

## Configuration

### API Key Setup

1. Create a [Tripo Developers account](https://developers.tripo3d.ai/en/auth/register).
2. Generate a key on the [API Keys page](https://developers.tripo3d.ai/en/keys).
3. Enter and confirm the key in **Tools > Tripo API**, or pass it to the runtime
   Blueprint functions.

The plugin uses Tripo API v3 at `https://openapi.tripo3d.ai/v3`. See
[TripoApiV3.md](Documentation/TripoApiV3.md) for the versioned API contract and
remaining integration verification work.

## Support and Community

- [Tripo Developers](https://developers.tripo3d.ai/)
- [API Documentation](https://developers.tripo3d.ai/en/docs/introduction)
- [Tripo DCC Bridge Guide Hub](https://www.tripo3d.ai/blog/tripo-dcc-bridge-guide-hub)
- Email: support@tripo3d.ai
- [Official Website](https://www.tripo3d.ai)

## Contributing

Contributions are welcome. Please report bugs, suggest features, or submit a
pull request.

## Version History

- v1.0.1 (Current)
  - Introduced the Tripo API v3 Unreal source package.
  - Added native Editor text/image generation, task queue, FBX import, and
    runtime Blueprint API integration.
  - Renamed user-facing metadata to **Tripo API Unreal** and author metadata to
    **TripoAI**.
  - Releases `Tripo_API_Unreal-V<version>.zip` source packages that Unreal
    compiles for the installed engine version.
  - Routed every model import through a single `TripoAssetPaths` header so assets
    land in `/Game/TripoModels`, matching the Tripo3D-UE-Bridge plugin. Staging
    stays at `Saved/TripoDownloads` and keeps the downloaded FBX.
  - Limited the API key confirmation tint to the key and check icons instead of
    the whole header bar.
  - Removed unused module dependencies, including `EditorStyle`, which was
    removed in UE 5.7 and blocked compilation there.
  - Corrected two include paths that only resolved on case-insensitive
    filesystems.
  - See [QA-V1.0.1.md](Documentation/QA-V1.0.1.md) for the upgrade summary, QA
    test matrix, and step-by-step install guide.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
