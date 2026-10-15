## What's new in MAKO Decky v4.6.0

<img src="https://raw.githubusercontent.com/eugeniosegala/MAKO/refs/heads/main/assets/neptune.png" alt="Neptune release artwork: the sea god raises his trident above a colossal mako in an epic battle of Renaissance fleets, with golden light breaking through storm clouds and painterly pixel textures" width="100%">

### Release codename: Neptune

> _“We held our course by the stars. Only at dawn did we see whose current carried us.”_
>
> **Mira Valen, _Charts of the Last Fleet_**

---

### Release highlight: Faster, leaner MAKO

Performance is the focus of MAKO 4.6 Neptune. **Faster Frame Generation, lower GPU memory use, and less CPU overhead for Scaling and Shaders** come with the matching Renderer, bringing the boost to your existing profiles without new settings to tune.

- **Faster Frame Generation:** The matching Renderer produced **about 3–11% more generated frames per second** in our standard benchmark tests compared with v4.5.0. One test using reduced Flow Scale gained **26.6%**. Both Fixed and Adaptive modes benefit, with FP16 or FP32, without a new setting.
- **Less GPU memory for Frame Generation:** MAKO used **about 28% less GPU memory** in one Quality-mode test with a high Frame Generation multiplier. Reusing temporary image storage helps keep memory use down at higher multipliers.
- **Less CPU work for Scaling:** MAKO reuses scaling preparation work when possible, reducing repeated CPU work while preserving your chosen scaling method and sharpness.
- **Less CPU work for Shaders:** The bundled shader system reduces temporary memory allocations during frame presentation. That means less CPU housekeeping, with the same effects and effect order.

**About these results:** We tested Frame Generation against v4.5.0 using the native Renderer on an AMD Steam Machine. The speed figures show how quickly MAKO can create extra frames; your in-game FPS improvement will depend on the game, hardware and settings. Memory figures cover only MAKO’s own GPU memory. The Scaling and Shader improvements reduce CPU work.

### Supporting improvements

- **Adaptive stability:** The matching Renderer avoids unnecessary resets of a proven Adaptive workload after brief image-availability misses during ordered presentation, while retaining backoff for unsuccessful higher-multiplier trials.
- **More complete translations:** Performance, HDR, scaling and Remote Play controls now use more consistent translated labels and explanations across MAKO Decky and the configuration app. Thanks to [Tak-attack](https://github.com/Tak-attack) for the Japanese improvements in [PR #91](https://github.com/eugeniosegala/MAKO/pull/91).
- **Better support reports:** Diagnostics retain Wayland protocol errors, and MAKO Wiz’s links open the correct guides from GitHub issue comments.
