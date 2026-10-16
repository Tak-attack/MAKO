# Collect MAKO Renderer Diagnostics

For standalone installations using `mako-launch`. If you use `mako-run`, follow the [MAKO Decky guide](../../plugin/docs/COLLECT_DIAGNOSTICS.md).

## 1. Enable logs

Fully close the game and save its current launch settings so you can restore them afterward.

**Steam or Proton:** prepend the two diagnostics variables to your existing Launch Options, preserving any profile, shader, or other settings. For a basic launch, the result is:

```text
MAKO_PRESENT_DIAGNOSTICS=1 MAKO_PRESENT_DIAGNOSTICS_THRESHOLD_MS=25 ~/.local/bin/mako-launch %command%
```

For a system installation, use your normal `mako-launch` path.

**Heroic, Lutris or Flatpak:** keep your working setup and add these per-game environment variables:

```text
MAKO_PRESENT_DIAGNOSTICS=1
MAKO_PRESENT_DIAGNOSTICS_THRESHOLD_MS=25
```

**Terminal launch:** capture the game output:

```bash
MAKO_PRESENT_DIAGNOSTICS=1 MAKO_PRESENT_DIAGNOSTICS_THRESHOLD_MS=25 \
    mako-launch your-game-command 2>&1 | tee "$HOME/Desktop/MAKO-renderer-session.log"
```

Replace `your-game-command` with your normal game command.

## 2. Reproduce the problem

Start the game, reproduce the issue once, note what happened and fully exit. Collect the report before another test run.

## 3. Save the report

In Desktop Mode, open Konsole and run:

```bash
mako-diagnostics --lines 5000 all > "$HOME/Desktop/MAKO-diagnostics.txt" 2>&1
```

If the command is not found, try `~/.local/bin/mako-diagnostics` instead. For the terminal capture above, add `--log "$HOME/Desktop/MAKO-renderer-session.log"` before `--lines`.

The file appears on your Desktop; Konsole normally prints nothing. If no log is found, check the temporary launch settings and repeat the test.

For a slowdown after a long session, replace `--lines 5000` with `--lines 200000`. Collect before starting another game.

Collect from the original session log, not an older filtered report, which may have omitted error details. The report does not include the separate Gamescope system journal; support may request it if the game log is insufficient.

For comparisons, collect a separate report after each run and note which settings changed. Use the same scene and game settings where possible. For HDR sharpening comparisons, also note the game resolution and display resolution. For VRR comparisons, record Steam’s VRR setting and the installed Gamescope version for each run. Bridge records identify the negotiated timing protocol; newer Gamescope can separate frame readiness, compositor waiting and explicit drops. Reported display timestamps remain predictions, so these logs alone cannot prove physical scanout cadence.

The initial `presentation timing initialized` record identifies optional Renderer-owned `VK_EXT_present_timing` support on a non-bridge surface. `active=0` means initialization retained existing pacing; a later `presentation timing fallback` record means feedback or clock support became unavailable. This capability depends on the installed driver and WSI; MAKO does not update Gamescope.

## 4. Restore your settings

Restore the original launch settings and remove the two temporary diagnostics variables.

## 5. Send the report

Review `MAKO-diagnostics.txt` and remove personal information. Upload it through the [diagnostic form](https://docs.google.com/forms/d/e/1FAIpQLScSd9qgkYCq3Kbbc3_52k4_82iTmEqt3_FxOqGuxQ6FsjutgA/viewform), choosing **MAKO Renderer (standalone/direct installation)**. Answer **Unknown** when unsure. Do not post the log publicly or attach `Lossless.dll`.
