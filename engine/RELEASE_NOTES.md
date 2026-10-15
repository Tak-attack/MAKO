## What's new in MAKO Renderer v4.6.0

<img src="https://raw.githubusercontent.com/eugeniosegala/MAKO/refs/heads/main/assets/neptune.png" alt="Neptune release artwork: the sea god guides a colossal mako and a sailing fleet along a turquoise current toward a sunlit coastal city" width="100%">

### Release codename: Neptune

> _“At Neptune’s command, the ocean became a road, and the mako its swiftest messenger.”_
>
> **Tavian Rook, _Annals of the Deep Court_**

---

Neptune reduces the cost of Frame Generation, Scaling and Shaders, with an Adaptive stability fix and more complete translations.

- **Lower Frame Generation GPU cost:** Independent processing stages can run together with fewer synchronization barriers, preserving the model’s calculations and selected precision. The optimization applies to Fixed and Adaptive modes with FP16 or FP32.
- **Less Frame Generation memory:** Generated frames share temporary GPU images, and later processing stages reuse scratch images after their earlier work finishes. Higher multipliers no longer need a separate copy of these temporary images for every generated frame.
- **Lower Scaling CPU overhead:** Reuse completed scaling commands when their source and destination images are unchanged. Scaling methods, sharpness and image calculations keep their existing behavior.
- **Lower Shader CPU overhead:** The bundled vkBasalt avoids temporary heap allocations during ordinary presentation. Effect algorithms and ordering are unchanged; shader GPU cost still depends on the selected effects and Frame Generation multiplier.
- **Adaptive stability:** Brief image-availability misses during ordered presentation preserve an established Adaptive workload instead of unnecessarily resetting it. Unsuccessful higher-multiplier trials still back off.
- **More complete translations:** Performance, HDR, scaling and Remote Play controls have more consistent translations across the configuration app and MAKO Decky. Bundled shader names and the file picker’s “All Files” filter now follow the selected language. Thanks to [Tak-attack](https://github.com/Tak-attack) for the Japanese improvements in [PR #91](https://github.com/eugeniosegala/MAKO/pull/91).
- **Better diagnostics:** Reports retain Wayland protocol errors that can explain compositor disconnects and scaling failures.

Performance gains vary with the model, resolution, multiplier, GPU and game; lower processing cost does not guarantee the same increase in game FPS.
