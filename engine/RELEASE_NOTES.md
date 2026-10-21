## What's new in MAKO Renderer v4.6.0

<img src="https://raw.githubusercontent.com/eugeniosegala/MAKO/refs/heads/main/assets/tempest.png" alt="Tempest release artwork: the blue-and-white MAKO shark circles a ruined temple where a sea god raises his trident, surrounded by battling ships and a vast whirlpool beneath golden storm light, in a Renaissance painting with pixel-art textures" width="100%">

### Release codename: Tempest

> _“The tempest did not break around the mako. It broke wherever the mako passed.”_
>
> **Tavian Rook, _Annals of the Tempest Court_**

---

Tempest improves Frame Generation speed, GPU memory use and frame pacing, with per-game shader tuning, shared MangoHud layouts and clearer diagnostics.

<img src="https://raw.githubusercontent.com/eugeniosegala/MAKO/refs/heads/main/assets/tempest-benchmarks.png" alt="MAKO 4.5 and 4.6 compared on AMD Steam Machine: paired slate and cyan bars normalize each 4.5 baseline to 100%, with original FPS, GPU milliseconds and private buffer MiB values plus percentage changes; the 800p 2× memory controls are unchanged" width="100%">

_Native source-build comparison on AMD Steam Machine (NAVI33/RADV). Synthetic results; game FPS and Steam Deck gains were not measured. Buffer memory excludes model weights, shared images, driver memory and game VRAM._

- **GPU Headroom:** A new live Performance control in the configuration app offers 5%, 10%, 15% or 20% spare GPU capacity targets during Frame Generation. It gradually reduces real FPS using supported AMD utilization readings; Adaptive can use more generated frames within its ceiling, while Fixed can lower output FPS. Off by default, with no Headroom sampling or extra cap when left Off. Gamescope is optional; brief spikes and GPU-load limits can prevent the target from being reached.
- **Faster Frame Generation:** Lower CPU and GPU overhead benefits Fixed and Adaptive with FP16 or FP32. The chart above compares Quality and Lighter throughput and GPU time with v4.5.0. In-game gains depend on the game, hardware and settings.
- **More efficient FP16 Quality:** Supported native FP16 Quality models use a leaner compute path on AMD’s RADV driver, preserving model weights, precision and Flow Scale without adding image buffers. It applies to Fixed and Adaptive, bridge and non-bridge, with VRR on or off. Enable **Allow FP16** and leave **Lighter FG Model** and **Ultra Performance** off. Steam Deck and other AMD/RADV handhelds can use this path; their performance gains have not yet been measured.
- **Lower overhead for Quality and Lighter:** Fixed 2× and Adaptive steps that produce one generated frame now submit preprocessing and generation together, reducing CPU submission cost and gaps between GPU stages. This works with FP16 or FP32, in bridge and non-bridge modes, preserving model quality and Flow Scale. Larger generation batches retain their existing path.
- **Lower FG working-buffer memory:** Generated outputs share temporary image storage, reducing private buffer allocations at higher multipliers. The chart includes unchanged 800p 2× controls and measures FG working buffers rather than total MAKO or game VRAM.
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
