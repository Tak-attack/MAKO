## What's new in MAKO Decky v4.6.0

<img src="https://raw.githubusercontent.com/eugeniosegala/MAKO/refs/heads/main/assets/tempest.png" alt="Tempest release artwork: the blue-and-white MAKO shark circles a ruined temple where a sea god raises his trident, surrounded by battling ships and a vast whirlpool beneath golden storm light, in a Renaissance painting with pixel-art textures" width="100%">

### Release codename: Tempest

> _“By dawn, the fleet understood: the tempest was a road with teeth.”_
>
> **Mira Valen, _Charts of the Drowned Horizon_**

---

### Release highlight: Faster, leaner MAKO

Performance is the focus of MAKO 4.6 Tempest. **Faster Frame Generation, lower GPU memory use, and less CPU overhead for Scaling and Shaders** come with the matching Renderer, bringing the boost to your existing profiles without new settings to tune.

- **Faster Frame Generation:** The matching Renderer produced **about 3–11% more generated frames per second** in our standard benchmark tests compared with v4.5.0. One test using reduced Flow Scale gained **26.6%**. Both Fixed and Adaptive modes benefit, with FP16 or FP32, without a new setting.
- **Less GPU memory for Frame Generation:** MAKO used **about 28% less GPU memory** in one Quality-mode test with a high Frame Generation multiplier. Reusing temporary image storage helps keep memory use down at higher multipliers.
- **Less CPU work for Scaling:** MAKO reuses scaling preparation work when possible, reducing repeated CPU work while preserving your chosen scaling method and sharpness.
- **Less CPU work for Shaders:** The bundled shader system reduces temporary memory allocations during frame presentation. That means less CPU housekeeping, with the same effects and effect order.

**About these results:** We tested Frame Generation against v4.5.0 using the native Renderer on an AMD Steam Machine. The speed figures show how quickly MAKO can create extra frames; your in-game FPS improvement will depend on the game, hardware and settings. Memory figures cover only MAKO’s own GPU memory. The Scaling and Shader improvements reduce CPU work.

### Customize shaders per game

Select **Customize shaders** in MAKO Decky to fine-tune selected bundled and compatible custom effects. Adjust shader parameters with sliders, toggles, choices, and colour controls, or use **Reset shader to defaults** to start again with the selected effect.

Your adjustments save with the selected profile and are shared across its AC and battery modes. Changes apply live when Shaders was enabled at game launch; rebuilding an effect may cause a brief hitch. The editor opens beside the game, and **Hide controls** collapses it for a clearer view while comparing results.

Focused controls stay clear of the editor border and scrollbar. Unavailable shader actions are skipped during controller navigation.

### Supporting improvements

- **Shared MangoHud layouts:** Profiles with MangoHud enabled can use an optional `~/.config/mako-render/mangohud.conf`, shared with the standalone Renderer launcher. Creating or removing the file takes effect on the next game launch without resaving profiles. Keep the usual `mako-run %command%` launch option. Requires host-installed MangoHud; Flatpak games and simultaneous Shaders remain unsupported.
- **Native FPS above the Adaptive target:** With **Fractional Adaptive** and **Automatic** Real Frame Priority, sustained native FPS can exceed **Target FPS** without generation when **Base FPS Cap** is off or at least the target. Manual caps remain enforced.
- **Adaptive stability:** The matching Renderer avoids unnecessary resets of a proven Adaptive workload after brief image-availability misses during ordered presentation. Smooth Cadence requires sustained delivery before accepting a lower multiplier, while unsuccessful higher-multiplier trials retain their backoff.
- **Gamescope frame pacing:** The matching Renderer keeps scaling feedback flowing while the game or driver is idle, corrects VRR timing handling, and avoids unnecessary delays after presentation waits.
- **More complete translations:** Performance, HDR, scaling and Remote Play controls now use more consistent translated labels and explanations across MAKO Decky and the configuration app. Thanks to [Tak-attack](https://github.com/Tak-attack) for the Japanese improvements in [PR #91](https://github.com/eugeniosegala/MAKO/pull/91).
- **Better support reports:** With diagnostics enabled, MAKO Decky retains ten launch sessions instead of five. Reports preserve Wayland protocol errors and multi-line failure details even in Steam’s timestamped logs, and MAKO Wiz’s links open the correct guides from GitHub issue comments.
