## What's new in MAKO Renderer v4.6.0

<img src="https://raw.githubusercontent.com/eugeniosegala/MAKO/refs/heads/main/assets/neptune.png" alt="Neptune release artwork: the blue-and-white MAKO shark circles a ruined temple where Neptune raises his trident, surrounded by battling ships and a vast whirlpool beneath golden storm light, in a Renaissance painting with pixel-art textures" width="100%">

### Release codename: Neptune

> _“At Neptune’s command, the ocean became a road, and the mako its swiftest messenger.”_
>
> **Tavian Rook, _Annals of the Deep Court_**

---

### Release highlight: Faster, leaner MAKO

Performance is the focus of MAKO 4.6 Neptune: **faster Frame Generation, lower GPU memory use, and less CPU overhead for Scaling and Shaders.** Get more from the MAKO stack with your existing settings.

- **Faster Frame Generation:** MAKO produced **about 3–11% more generated frames per second** in our standard benchmark tests compared with v4.5.0. One test using reduced Flow Scale gained **26.6%**. Both Fixed and Adaptive modes benefit, with FP16 or FP32.
- **Less GPU memory for Frame Generation:** MAKO used **about 28% less GPU memory** in one Quality-mode test with a high Frame Generation multiplier. Reusing temporary image storage helps keep memory use down at higher multipliers.
- **Less CPU work for Scaling:** MAKO reuses scaling preparation work when possible, reducing repeated CPU work while preserving your chosen scaling method and sharpness.
- **Less CPU work for Shaders:** The bundled shader system reduces temporary memory allocations during frame presentation. That means less CPU housekeeping, with the same effects and effect order.

**About these results:** We tested Frame Generation against v4.5.0 using the native Renderer on an AMD Steam Machine. The speed figures show how quickly MAKO can create extra frames; your in-game FPS improvement will depend on the game, hardware and settings. Memory figures cover only MAKO’s own GPU memory. The Scaling and Shader improvements reduce CPU work.

### Supporting improvements

- **Adaptive stability:** Brief image-availability misses during ordered presentation preserve an established Adaptive workload instead of unnecessarily resetting it. Unsuccessful higher-multiplier trials still back off.
- **More complete translations:** Performance, HDR, scaling and Remote Play controls have more consistent translations across the configuration app and MAKO Decky. Bundled shader names and the file picker’s “All Files” filter now follow the selected language. Thanks to [Tak-attack](https://github.com/Tak-attack) for the Japanese improvements in [PR #91](https://github.com/eugeniosegala/MAKO/pull/91).
- **Better diagnostics:** Reports retain Wayland protocol errors that can explain compositor disconnects and scaling failures.
