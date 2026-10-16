# Configuration

**MAKO Renderer Configuration** (`mako-ui`) saves automatically. Options marked **Restart** need a game restart. MAKO Decky shares the same profiles; edit them in one UI at a time.

## Quick start

1. Open **MAKO Renderer Configuration** from the application launcher or run `~/.local/bin/mako-ui`.
2. Create a profile and add the game's executable under **Matched Processes**. Or start the game, use **Detect Running Game…**, save the match, and close it.
3. Choose Frame Generation, Scaling, and/or Shaders.
4. Copy the complete launch option shown by the UI into the game's Steam properties, then launch the game again.

Detection finds the executable; MAKO activates on the next launch. The configuration window can be closed during play. Try display modes and V-Sync on/off if a feature does not work smoothly.

## Lossless Scaling availability

**MAKO Scaler and Shaders work without Lossless Scaling.** Frame Generation and LS1 require it. If MAKO reports it missing, install Lossless Scaling or correct its DLL path, then restart the game.

## Profiles

**Create New Profile** copies the selected profile's settings, matches, power modes, and shaders. Use **Rename Profile** or **Delete Profile** to manage copies; **Default** is protected.

The dropdown selects the profile you edit; local games use their executable matches. If detection fails, use **Show all applications** or add the executable manually. Match the game, not its launcher, Steam title, or ROM filename.

### AC and battery settings

Under **Power Profiles**, enable **Separate power settings**, then choose Handheld, Docked, or Base under **Editing settings for**.

Battery selects Handheld; charging selects Docked without requiring a dock. An external display alone does not switch sets. Base applies if startup power detection fails or separate settings are off. DLL, FP16, Shaders, and launcher settings stay shared; normal live/restart rules apply.

### Launcher exclusions

Known launchers and helpers are excluded so their child games can match normally. If detection finds only a launcher, wait for gameplay and try again.

## Native Steam Remote Play

On the receiving device, close streams and select **Override Remote Play**. Choose a saved profile before streaming. **Restore Steam Client** removes the override; **Refresh** checks its status. See the [Remote Play guide](../../plugin/docs/REMOTE-PLAY.md) for setup and limits.

## Profile settings

### Frame Generation

Turn on **Enable Frame-gen (Restart)** before launching. **Real FPS** counts game frames; output FPS includes generated frames. In Fixed or Adaptive mode, `0x` pauses generation and its FPS caps live; selecting 2x–5x resumes it.

| Control | What it does |
| --- | --- |
| **Fixed Multiplier** | Requests 2x–5x output. Start at 2x. |
| **Adaptive Frame Generation** | Varies generation toward **Target FPS** (10–1000), up to **Maximum Adaptive Multiplier** (2x–5x). |
| **Match Display Refresh Rate** | Uses Gamescope's refresh as the Adaptive target; **Fallback Target FPS** applies without feedback. Turn matching off to edit the target. |
| **Steady Base Cap** | Initially caps real FPS at half the target for even cadence; may reduce responsiveness. |
| **Fractional Adaptive** | Keeps more real frames, but delivery may feel less even. |
| **Real Frame Priority** | Low–Very High select a Fractional real-FPS cap; higher priority allows more real FPS. **Automatic** adds no priority cap. |
| **Base FPS Cap** | Limits real FPS. **Off** adds no manual cap; Steady or explicit Fractional caps take precedence. |
| **Smooth Cadence** | Favours consistent delivery; compare on/off for responsiveness. |
| **Gamescope VRR** | Follows Steam, or temporarily requests VRR on/off on a supported display during play. |

With **Fractional Adaptive** and **Automatic** Real Frame Priority, sustained native FPS can exceed **Target FPS** without generation when **Base FPS Cap** is off or at least the target. For example, a 60 FPS target on a 90 Hz display allows native 75 FPS gameplay with the cap off or set to 90; the manual cap still limits faster sources. Smooth Cadence can be on or off; below-target caps, explicit priorities and Steady keep their selected pacing.

For games switching between 30 FPS cutscenes and 60 FPS gameplay on a 60 Hz display, try Fractional Adaptive with a 2x ceiling and a 60 FPS target, **Real Frame Priority** set to **Automatic**, **Base FPS Cap** off, and **Dynamic Cadence Recovery** enabled. Recovery briefly removes generated work to detect a faster native rate hidden by presentation pacing; its checks can lower output FPS or hitch. **Match Display Refresh Rate** can supply the target when Gamescope confirms 60 Hz. **Auto-disable Frame Generation by Refresh Rate** instead checks display Hz: a 50 Hz threshold allows generation on a 60 Hz display regardless of the game's real FPS. It is not a native-FPS cutoff.

### Scaling

Enable Scaling before launching. Set Steam's **Game Resolution** to the display maximum, then choose a lower in-game resolution. Gamescope is recommended; try different display modes if needed.

| Method | Use it for |
| --- | --- |
| **Native Resolution** | Simple model-free reconstruction |
| **MAKO Scaler** | Open scaling with anti-ringing and sharpening |
| **LS1 Quality** | Highest-quality LS1 scaling; requires Lossless Scaling |
| **LS1 Performance** | Lower-cost LS1 scaling; requires Lossless Scaling |

**Scale Factor** targets 1.0x–2.0x output dimensions, within display limits; **Sharpness** adjusts sharpening except in Native Resolution. **Quality Supersampling** can improve quality on supported Gamescope surfaces, with higher GPU and memory use. LS1 failures fall back to MAKO Scaler.

### Performance and device selection

| Control | What it does |
| --- | --- |
| **Ultra Performance (Restart)** | Uses 70% Flow Scale, the lighter FG model, and LS1 Performance when scaling is enabled. |
| **Flow Scale** | 25–100% motion-estimation resolution; lower saves GPU work at a quality cost. Ultra Performance fixes it at 70%. |
| **Lighter FG Model** | Reduces GPU work, with potentially more artifacts; forced on by Ultra Performance. |
| **Allow FP16 (Restart)** | Global precision: FP16 for supported Frame Generation models and MAKO Scaler; off uses FP32. LS1 always uses FP32. |
| **Lossless.dll Path (Restart)** | Global override; leave empty for automatic detection. |
| **GPU (Restart)** | Selects a GPU; multi-GPU Frame Generation is unsupported. |

### Qt Shaders and compatibility controls

Enable **Shaders** before launching to use bundled vkBasalt; more effects cost more GPU time.

- **Effects:** select effects in order; uncheck and recheck to move one last. **Off** clears the effect list. **HDR Look (SDR)** changes appearance without enabling HDR.
- **Sharpening:** Off, CAS, or DLS, with adjustable **Sharpness**. **DLS denoise** limits sharpening of grain and noise.
- **Anti-aliasing:** Off, FXAA, or SMAA.

Select **Customize shaders** to adjust selected bundled and compatible custom effects with sliders, exact-value fields, toggles, choices, and colour controls. **Reset shader to defaults** resets only the selected effect, preserving values its author marked as non-resettable. The popup edits parameters; choose enabled effects in **Effects**.

Adjustments save with the selected profile and are shared across AC and battery modes. Copies retain them; deleting the profile removes them. Unchecking a custom shader retains its values, while deleting its registration removes them. Changes apply live when Shaders was enabled at game launch. Rebuilding an effect may briefly hitch, and higher quality or iteration settings can increase GPU cost.

The movable editor opens on the left, stays above other windows, and temporarily hides the main configuration window. **Hide controls** collapses it for a clearer view of the game. Closing restores the main window and waits for pending saves; failed saves can be retried or discarded.

If the editor requests an update, install the matching Renderer. Custom effects must be vkBasalt-compatible; compile-time macros are not editable here. Reopen the editor after changing shader source or includes. See the [shader editor contract](LAYER-CHAINING.md#shader-parameter-editor-contract) for supported parameter types and implementation details.

**Add custom shader** registers a vkBasalt-compatible ReShade `.fx` file; enable it in **Effects**. Keep includes and textures accessible to Flatpak games. **Delete selected custom shaders** removes profile registrations, preserving original files. **Refresh** reloads external edits; failed loads keep the previous chain active.

Under **Compatibility**, keep defaults unless a game needs a change:

- **Auto-disable Frame Generation by Refresh Rate / Refresh Rate Threshold:** pauses generation and Adaptive caps at or below the chosen Gamescope refresh; a manual Base FPS Cap remains.
- **Dynamic Cadence Recovery:** off by default. Rechecks games that change native FPS by briefly reducing generated work; these checks can lower output FPS or cause hitches under sustained GPU load. Clears base caps and resets Real Frame Priority. Enabling a cap or changing priority turns Recovery off.
- **Cadence Probe Interval:** 0.1–3 seconds between recovery checks; default 2 seconds. Shorter intervals react sooner but can hitch more often.
- **Game Swapchain Images (Restart):** may help games that fail to start, with less generated-frame headroom.

### Crop & Stretch

Select **Crop & Stretch** in **Effects**, then open **Customize shaders**. **Left crop (%)**, **Right crop (%)**, **Top crop (%)**, and **Bottom crop (%)** independently remove part of that edge and stretch the remaining image to fill the output. All four default to zero. For example, Top and Bottom at 5 remove 5% from each edge and stretch the remaining 90% vertically; Left and Right at zero keep the horizontal framing. **Reset shader to defaults** restores the original framing.

The controls apply live through the normal per-profile shader editor. Each edge is limited to 49% so opposing edges cannot cross. Cropping affects the whole game image, including HUD elements and subtitles, and independent horizontal/vertical stretching changes aspect ratio. It can remove bars within the game's image, but cannot remove borders added later by Gamescope or the display. Place it before other effects when those effects should use the stretched image.

### Desktop scaling and resolution

For 1920×1080 output at 1.5x, aim for a 1280×720 input. Raising the factor without lowering resolution can increase GPU and memory cost. Check source and presentation sizes in [diagnostics](COLLECT_DIAGNOSTICS.md); see [Scaling](SCALING.md) for supported display setups.

## Applying changes

| Timing | Common settings |
| --- | --- |
| **Restart** | Enabling Frame Generation, Scaling, or Shaders; DLL; FP16; GPU; Ultra Performance; launcher compatibility controls |
| **Live** | Most Frame Generation controls; scaling method and sharpness; Flow Scale; Lighter FG Model; shader adjustments |
| **May wait for resolution change or restart** | Scale Factor, Quality Supersampling, and changes needing more Frame Generation capacity |

Some live changes rebuild resources and may briefly hitch. See [Runtime transitions](RUNTIME-TRANSITIONS.md) for details.

**Interface Settings > Language** changes and saves the Qt interface language, including German and Russian. The first launch follows your system language, with English as the fallback.

## Launch Settings

For native Steam and Proton games, copy the UI's complete launch option. Without shaders or an explicit profile, the user-local archive uses:

```text
~/.local/bin/mako-launch %command%
```

The Arch package uses `/usr/bin/mako-launch`. For a terminal launch, replace `%command%` with the executable and arguments. Flatpak applications need [Flatpak preparation](FLATPAK-GUIDE.md).

The controls under **Launch Settings** apply globally to `mako-launch` games. **Disable Steam Overlay (Restart)** is on by default; it may reduce stutter but disables Steam's in-game overlay and FPS counter and may affect Steam Input. Gaming Mode menus and performance overlay remain available.

**Zink** runs OpenGL through Vulkan. **Force ALSA** changes the audio path for compatibility problems. Steam Deck mode, Gamescope WSI, and MangoHud controls are available in MAKO Decky.

## Advanced configuration

For manual TOML fields, defaults, and environment variables, see the [advanced reference](CONFIGURATION-REFERENCE.md). For problems, see [troubleshooting](TROUBLESHOOTING.md) or [collect diagnostics](COLLECT_DIAGNOSTICS.md).

## HDR

**HDR is experimental and disabled by default.** It works with Scaling, Frame Generation and Shaders in Gaming Mode on Steam Deck OLED or an HDR-capable external display, with HDR allowed in Gamescope. The Steam Deck LCD’s built-in display is SDR.

In **Launch Settings**, turn on **Enable HDR (Restart)**, restart the game, then enable HDR in its settings. Restart after changing MAKO’s HDR toggle or switching between HDR and SDR displays.

**HDR Quality** defaults to **Performance**. **Quality** retains full precision and full motion analysis; **Balanced** uses compact shader/scaling buffers while retaining linear-light Frame Generation and full motion analysis; **Performance** also uses approximate HDR10 generation and coarser motion analysis. Output resolution stays unchanged. Changes apply live with a brief hitch; lower precision may add banding or motion artifacts. See the [HDR pipeline](HDR-PIPELINE.md) for details.
