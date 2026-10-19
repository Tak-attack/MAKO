## What's new in MAKO Decky v4.6.0

<img src="https://raw.githubusercontent.com/eugeniosegala/MAKO/refs/heads/main/assets/tempest.png" alt="Tempest release artwork: the blue-and-white MAKO shark circles a ruined temple where a sea god raises his trident, surrounded by battling ships and a vast whirlpool beneath golden storm light, in a Renaissance painting with pixel-art textures" width="100%">

### Release codename: Tempest

> _“By dawn, the fleet understood: the tempest was a road with teeth.”_
>
> **Mira Valen, _Charts of the Drowned Horizon_**

---

Tempest brings faster, leaner Frame Generation and steadier pacing through the matching Renderer, alongside per-game shader tuning, smoother controller navigation and clearer support reports.

- **Faster Frame Generation:** The matching Renderer reduces CPU and GPU overhead in Fixed and Adaptive with FP16 or FP32. In native benchmarks on an AMD Steam Machine, FP16 Quality at 2× generation and 100% Flow Scale produced **about 11% more generated frames per second at 1440p and 9% more at 4K** than v4.5.0, with **about 10% and 6% less GPU time per generated frame**, respectively. In-game gains depend on the game, hardware and settings.
- **More efficient FP16 Quality:** The matching Renderer automatically uses a leaner compute path for supported native FP16 Quality models on AMD’s RADV driver, preserving model weights, precision and Flow Scale without adding image buffers. It applies to Fixed and Adaptive, bridge and non-bridge, with VRR on or off. Enable **Allow FP16** and leave **Lighter FG Model** and **Ultra Performance** off. Steam Deck and other AMD/RADV handhelds can use this path; their performance gains have not yet been measured.
- **Lower GPU memory use:** Reusing temporary image storage reduced MAKO’s own GPU memory use by **about 28%** in one Quality-mode test on the same machine, with a high Frame Generation multiplier. The figure covers MAKO’s allocations only.
- **Less CPU work for Scaling and Shaders:** The matching Renderer reuses scaling preparation, and the bundled shader system reduces temporary allocations during frame presentation. Your scaling method, sharpness, shader effects and effect order are preserved.
- **Adaptive and FIFO stability:** The matching Renderer preserves proven Adaptive workloads after brief image-availability misses and retains qualified Smooth Cadence pacing through short cadence dips. Sustained slowdowns and unsuccessful higher-multiplier trials still trigger the existing safeguards.
- **More reliable multiplier checks:** Adaptive checks the real-frame cost after pacing settles and refreshes older comparisons when needed. Brief bursts of queued frames no longer repeatedly restart these checks. When both workloads reach the target, MAKO prefers the lower multiplier.
- **Better use of frame-cap waits:** Eligible capped Fixed and Adaptive paths coordinate frame preparation with output timing, including supported Scaling and bridge paths, avoiding consecutive pacing waits while preserving your configured cap.
- **Gamescope timing compatibility:** The matching Renderer improves bridge feedback handling, VRR timing interpretation and presentation deadlines. Newer Gamescope reports and optional Vulkan timing on non-bridge surfaces are used when supported, with existing pacing retained otherwise. This does not require replacing SteamOS’s Gamescope.
- **Customize shaders per game:** Select **Customize shaders** to adjust selected bundled and compatible custom effects with sliders, toggles, choices and colour controls. **Reset shader to defaults** restores the selected effect.
- **Live shader adjustments:** Changes save with the selected profile and are shared across its AC and battery modes. They apply live when Shaders was enabled at game launch; rebuilding an effect may cause a brief hitch. The editor opens beside the game, and **Hide controls** collapses it while comparing results.
- **Clearer shader controls:** Palettes such as Nostalgia’s **Custom palette** use colour swatches, sliders and exact-value fields. Sliders follow the author’s step size with consistent controller increments. Numeric fields hide floating-point display noise without changing stored values or saving untouched fields.
- **Smoother controller navigation:** Focused controls stay visible when entering settings tabs or leaving the effects list. Collapsing the list reveals the next focused action without restoring stale scroll positions. Unavailable actions are skipped, and closing the shader editor returns to the selected MAKO section in Decky’s Quick Access panel.
- **Shared MangoHud layouts:** Profiles with MangoHud enabled can use an optional `~/.config/mako-render/mangohud.conf`, shared with the standalone Renderer launcher. Creating or removing it takes effect on the next game launch without resaving profiles. Keep the usual `mako-run %command%` launch option. Requires host-installed MangoHud; Flatpak games and simultaneous Shaders remain unsupported.
- **Native FPS above the Adaptive target:** With **Fractional Adaptive** and **Automatic** Real Frame Priority, sustained native FPS can exceed **Target FPS** without generation when **Base FPS Cap** is off or at least the target. Manual caps remain enforced.
- **More complete translations:** Performance, HDR, scaling and Remote Play controls have more consistent translated labels and explanations across MAKO Decky and the configuration app. Thanks to [Tak-attack](https://github.com/Tak-attack) for the Japanese improvements in [PR #91](https://github.com/eugeniosegala/MAKO/pull/91).
- **Better support reports:** With diagnostics enabled, MAKO Decky retains ten launch sessions instead of five. Reports preserve Wayland errors and multi-line failure details in Steam’s timestamped logs, identify failed presentation waits and available timing capabilities, and keep MAKO Wiz’s guide links usable from GitHub issue comments.
