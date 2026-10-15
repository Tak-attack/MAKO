## What's new in MAKO Decky v4.6.0

<img src="https://raw.githubusercontent.com/eugeniosegala/MAKO/refs/heads/main/assets/neptune.png" alt="Neptune release artwork: the sea god guides a colossal mako and a sailing fleet along a turquoise current toward a sunlit coastal city" width="100%">

### Release codename: Neptune

> _“We held our course by the stars. Only at dawn did we see whose current carried us.”_
>
> **Mira Valen, _Charts of the Last Fleet_**

---

Neptune brings the matching Renderer’s performance improvements to existing profiles, alongside clearer translated controls and better support reports.

- **Frame Generation efficiency:** The matching Renderer reduces GPU processing overhead and shares temporary image memory across generated frames. Fixed and Adaptive modes benefit with FP16 or FP32, without a new setting.
- **Scaling overhead:** Reusing completed scaling commands reduces CPU work while retaining your chosen scaling method and sharpness.
- **Shader overhead:** The bundled vkBasalt reduces temporary CPU allocations during ordinary presentation. Your effect selection and order are preserved; GPU cost still depends on the effects and Frame Generation multiplier.
- **Adaptive stability:** The matching Renderer avoids unnecessary resets of a proven Adaptive workload after brief image-availability misses during ordered presentation, while retaining backoff for unsuccessful higher-multiplier trials.
- **More complete translations:** Performance, HDR, scaling and Remote Play controls now use more consistent translated labels and explanations across MAKO Decky and the configuration app. Thanks to [Tak-attack](https://github.com/Tak-attack) for the Japanese improvements in [PR #91](https://github.com/eugeniosegala/MAKO/pull/91).
- **Better support reports:** Diagnostics retain Wayland protocol errors, and MAKO Wiz’s links open the correct guides from GitHub issue comments.

Performance gains vary by workload and hardware; this release does not promise a fixed increase in game FPS.
