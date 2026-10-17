## What's new in MAKO Renderer v4.6.0

<img src="https://raw.githubusercontent.com/eugeniosegala/MAKO/refs/heads/main/assets/tempest.png" alt="Tempest release artwork: the blue-and-white MAKO shark circles a ruined temple where a sea god raises his trident, surrounded by battling ships and a vast whirlpool beneath golden storm light, in a Renaissance painting with pixel-art textures" width="100%">

### Release codename: Tempest

> _“The tempest did not break around the mako. It broke wherever the mako passed.”_
>
> **Tavian Rook, _Annals of the Tempest Court_**

---

Tempest improves Frame Generation speed, GPU memory use and frame pacing, with per-game shader tuning, shared MangoHud layouts and clearer diagnostics.

- **Faster Frame Generation:** The native Renderer produced **about 3–11% more generated frames per second** than v4.5.0 in standard benchmarks on an AMD Steam Machine; one reduced Flow Scale test gained **26.6%**. Fixed and Adaptive benefit with FP16 or FP32. These figures measure frame-generation throughput; in-game gains depend on the game, hardware and settings.
- **Lower GPU memory use:** Reusing temporary image storage reduced MAKO’s own GPU memory use by **about 28%** in one Quality-mode test on the same machine, with a high Frame Generation multiplier. The figure covers MAKO’s allocations only.
- **Less CPU work for Scaling and Shaders:** Scaling reuses preparation work, and the bundled shader system reduces temporary allocations during frame presentation. Your scaling method, sharpness, shader effects and effect order are preserved.
- **Adaptive and FIFO stability:** Brief image-availability misses preserve a proven Adaptive workload. Smooth Cadence retains qualified FIFO pacing through short cadence dips. Sustained slowdowns and unsuccessful higher-multiplier trials still trigger the existing safeguards.
- **More reliable multiplier checks:** Adaptive compares real-frame rate and delivered output after pacing settles, refreshing older baselines when needed. Brief bursts of queued frames no longer repeatedly restart the comparison. When both workloads reach the target, MAKO prefers the lower multiplier.
- **Better use of frame-cap waits:** Eligible capped Fixed and Adaptive paths prepare generated output during the existing source-frame wait, including supported Scaling and bridge paths. Output timing stays coordinated while preserving the configured cap and application presentation waits.
- **Gamescope and Vulkan timing:** The bridge keeps feedback flowing during game or driver idle periods, corrects VRR timing interpretation and reuses ready presentation deadlines. Newer Gamescope reports and optional Vulkan timing on non-bridge surfaces are used when supported, with existing pacing retained otherwise. Gamescope remains optional for desktop SDR operation.
- **Customize shaders per game:** Open **Customize shaders** in the configuration app to adjust selected bundled and compatible custom effects with sliders, toggles, choices and colour controls. **Reset shader to defaults** restores the selected effect.
- **Live shader adjustments:** Changes save with the selected profile and are shared across its AC and battery modes. They apply live when Shaders was enabled at game launch; rebuilding an effect may cause a brief hitch.
- **Clearer shader controls:** Palettes such as Nostalgia’s **Custom palette** use colour swatches, sliders and exact-value fields. Sliders respect the author’s step size, with sensible decimal increments when none is provided. Numeric fields show cleaner values without changing stored precision or saving untouched fields.
- **More usable shader editor:** The movable editor stays above the game, and **Hide controls** collapses it while comparing results. Controls stay clear of scrollbars and window edges, keeping Reset accessible in narrower windows.
- **MangoHud in the configuration app:** Enable host-installed MangoHud per profile under **External Tools**, then copy the updated Steam launch option and restart the game. The selection is shared with MAKO Decky. Both launchers support an optional `~/.config/mako-render/mangohud.conf` layout. Standalone activation requires Python 3; MangoHud remains exclusive with Shaders and unavailable for Flatpak games.
- **Native FPS above the Adaptive target:** With **Fractional Adaptive** and **Automatic** Real Frame Priority, sustained native FPS can exceed **Target FPS** without generation when **Base FPS Cap** is off or at least the target. Manual caps remain enforced.
- **More complete translations:** Performance, HDR, scaling and Remote Play controls have more consistent translations across the configuration app and MAKO Decky. Bundled shader names and the file picker’s “All Files” filter follow the selected language. Thanks to [Tak-attack](https://github.com/Tak-attack) for the Japanese improvements in [PR #91](https://github.com/eugeniosegala/MAKO/pull/91).
- **Better diagnostics:** Reports retain Wayland errors and multi-line failure details in Steam’s timestamped logs. Presentation-wait summaries identify the first failed or timed-out call, its requested timeout and actual duration; timing records identify available compositor capabilities. The collector can select from all ten launch sessions retained by MAKO Decky when diagnostics are enabled.
