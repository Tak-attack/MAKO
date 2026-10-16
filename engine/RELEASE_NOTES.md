## What's new in MAKO Renderer v4.6.0

<img src="https://raw.githubusercontent.com/eugeniosegala/MAKO/refs/heads/main/assets/tempest.png" alt="Tempest release artwork: the blue-and-white MAKO shark circles a ruined temple where a sea god raises his trident, surrounded by battling ships and a vast whirlpool beneath golden storm light, in a Renaissance painting with pixel-art textures" width="100%">

### Release codename: Tempest

> _“The tempest did not break around the mako. It broke wherever the mako passed.”_
>
> **Tavian Rook, _Annals of the Tempest Court_**

---

### Release highlight: Faster, leaner MAKO

Performance is the focus of MAKO 4.6 Tempest: **faster Frame Generation, lower GPU memory use, and less CPU overhead for Scaling and Shaders.** Get more from the MAKO stack with your existing settings.

- **Faster Frame Generation:** MAKO produced **about 3–11% more generated frames per second** in our standard benchmark tests compared with v4.5.0. One test using reduced Flow Scale gained **26.6%**. Both Fixed and Adaptive modes benefit, with FP16 or FP32.
- **Less GPU memory for Frame Generation:** MAKO used **about 28% less GPU memory** in one Quality-mode test with a high Frame Generation multiplier. Reusing temporary image storage helps keep memory use down at higher multipliers.
- **Less CPU work for Scaling:** MAKO reuses scaling preparation work when possible, reducing repeated CPU work while preserving your chosen scaling method and sharpness.
- **Less CPU work for Shaders:** The bundled shader system reduces temporary memory allocations during frame presentation. That means less CPU housekeeping, with the same effects and effect order.

**About these results:** We tested Frame Generation against v4.5.0 using the native Renderer on an AMD Steam Machine. The speed figures show how quickly MAKO can create extra frames; your in-game FPS improvement will depend on the game, hardware and settings. Memory figures cover only MAKO’s own GPU memory. The Scaling and Shader improvements reduce CPU work.

### Customize shaders per game

Open **Customize shaders** in the MAKO Renderer configuration app to fine-tune selected bundled and compatible custom effects. Adjust shader parameters with sliders, toggles, choices, and colour controls, or use **Reset shader to defaults** to start again with the selected effect.

Your adjustments save with the selected profile and are shared across its AC and battery modes. Changes apply live when Shaders was enabled at game launch; rebuilding an effect may cause a brief hitch. The movable editor stays above the game, and **Hide controls** collapses it for a clearer view while comparing results.

Controls now stay clear of the scrollbar and window edges, keeping the reset action accessible in narrower editor windows.

### Supporting improvements

- **MangoHud in the configuration app:** Enable host-installed MangoHud per profile under **External Tools**, then copy the updated Steam launch option and restart the game. The selection is shared with MAKO Decky. Both launchers can use an optional `~/.config/mako-render/mangohud.conf` for a shared layout. Standalone activation requires Python 3; MangoHud remains exclusive with Shaders and unavailable for Flatpak games.
- **Native FPS above the Adaptive target:** With **Fractional Adaptive** and **Automatic** Real Frame Priority, sustained native FPS can exceed **Target FPS** without generation when **Base FPS Cap** is off or at least the target. Manual caps remain enforced.
- **Adaptive stability:** Brief image-availability misses during ordered presentation preserve an established Adaptive workload instead of unnecessarily resetting it. Smooth Cadence requires sustained delivery before accepting a lower multiplier, avoiding downshifts based on a short burst. Unsuccessful higher-multiplier trials still back off.
- **Gamescope frame pacing:** Scaling keeps receiving compositor feedback while the game or driver is idle. Corrected VRR timing handling and reuse of ready presentation deadlines avoid unnecessary delays after presentation waits.
- **More complete translations:** Performance, HDR, scaling and Remote Play controls have more consistent translations across the configuration app and MAKO Decky. Bundled shader names and the file picker’s “All Files” filter now follow the selected language. Thanks to [Tak-attack](https://github.com/Tak-attack) for the Japanese improvements in [PR #91](https://github.com/eugeniosegala/MAKO/pull/91).
- **Better diagnostics:** Reports retain Wayland protocol errors and multi-line failure details even when Steam adds timestamps, helping explain compositor disconnects and scaling failures. The collector can select from all ten launch sessions retained by MAKO Decky when diagnostics are enabled.
