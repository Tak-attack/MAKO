# MAKO storage boundaries

Re-read current owning scripts on each run; environment overrides and layouts can change. Relative MAKO paths below start at the MAKO repository root.

## Keep reusable build inputs

| Storage | Reason to retain |
| --- | --- |
| `engine/build/cache/` | Flatpak data, `native-sdk`, `vulkan-headers`, `ccache`, `vkbasalt`, and `lsfg-fp16-tools`; preserve the whole cache |
| `MAKO_BUILD_CACHE_ROOT`, `MAKO_NATIVE_SDK_DIR`, `MAKO_FLATPAK_CACHE_ROOT` override paths | Relocated dependencies; resolve relative bases from the owning scripts |
| Docker/Podman `mako-flatpak-cache` volume, other verified build-cache volumes, and builder images | Portable Flatpak packaging mounts `/cache`; cache and work roots can both point there, so the name `work` does not make it disposable |
| `.flatpak-builder` directories, Flatpak installations, SDKs, runtimes, and downloads | Warm build environment, including `engine/.flatpak-builder` and `plugin/.flatpak-builder` |
| Host compilers, Qt, multilib, shader tools, Vulkan headers, container runtime, Node/pnpm/Python and shared environments | Regular build and release tooling; distinguish these from task-private experiment environments |
| Regular build/test `node_modules`, pnpm/npm stores, shared virtual environments, compiler/download caches, third-party sources and SDKs | Reusable dependencies; dependencies private to a completed disposable experiment are eligible with that environment |
| Actively used candidates and dependency overrides | `MAKO_VKBASALT_LOCAL_ARCHIVE`, for example, can point inside `engine/out`; inspect consumers before deletion |

Owners: `engine/docs/BUILDING-FROM-SOURCE.md`, `engine/scripts/package-local.sh`, `engine/scripts/package-flatpaks.sh`, `engine/scripts/build-steamos-dev.sh`, `plugin/docs/PACKAGING.md`, and `HOW_TO_RELEASE.md`. Inspect only relevant environment variables; do not dump the environment or secret-bearing config.

**Never use `engine/scripts/prune-build-cache.sh --confirm` or `engine/scripts/prune-steamos-flatpak-cache.sh --confirm` for this workflow. Both delete caches the user wants to keep.** Their default read-only reports do not implement this retention policy.

## Inspect for removal

| Area | Potentially disposable material | Required distinction |
| --- | --- | --- |
| Renderer | Incremental CMake trees, `engine/build/work`, binaries, packages and staging in `engine/out`, reviewed `engine/build-*` trees | Enumerate around caches; inspect nested dependencies and active candidate references |
| Decky | `plugin/out`, `plugin/dist`, `plugin/coverage`, generated reports, packaging staging and screenshots | Keep dependencies and originals not copied to their source owner |
| Website | `website/dist`, `dist-pages`, `.vinext`, `.next`, preview screenshots | Read `website/README.md`; preserve source, `public`, assets, and dependencies |
| vkBasalt | Meson/CMake output and completed package archives | Read its packaging scripts and ignore rules; keep downloaded source/toolchains |
| Other/experimental sibling repositories | Verified builds, packages, coverage, logs, temporary captures, abandoned task-private environments | Discover every checkout; retain source, Git history, regular build/test environments, and build caches |
| MAKO Gym | Completed run logs, QA images, obsolete extracted candidates, dated reports in `out`, and one-off test environments | Keep tools such as `out/renderer-comparison/tools`, SDK/runtime caches, workloads, and emulator environments when reused by the regular QA workflow. Include recent completed results and disposable experiment environments too, but disclose the loss of historical evidence and preserve active investigations |
| Codex | MAKO archived threads, unused images/charts/attachments, completed disposable worktrees | Follow the Codex procedure; establish ownership and lack of remaining consumers |

Prioritize generated test storage: CTest `Testing` output, sanitizer/test build trees, coverage and test reports, Gym comparison images, logs and captures, completed-run temporary fixtures, `__pycache__`, `.pytest_cache`, and verified MAKO-owned pytest temporary directories. Include completed, failed, and abandoned runs once they are no longer in use. Preserve tracked test cases and fixtures, curated baselines, and reusable test tools; distinguish those from disposable generated binaries and run data. Check tracked paths before proposing deletion. Ignore rules also protect licensed inputs, private evidence, and environments; ignored does not mean disposable.

## Temporary experiment environments

The user explicitly wants environments Codex created for completed Gym tests, release checks, and experiments cleaned up, even if recent. Classify by purpose and current consumers: keep regular build/release/test environments, but include abandoned task-private environments and their installed dependencies in the proposed removal set. A Python virtual environment, container, or sandbox is not protected merely because it contains tools.

Start in the discovered repositories, their generated output and worktrees, and task-owned output directories. Follow concrete script, task metadata, or environment-manager references into `/tmp`, `/var/tmp`, `TMPDIR`, Codex temporary directories, or other user-owned locations. Use bounded discovery for `.venv`, `venv`, `pyvenv.cfg`, Conda environment metadata, isolated dependency installs, Wine/Proton test prefixes, extracted test roots, and similar disposable sandboxes. These are discovery clues, not deletion patterns. Do not scan or remove unrelated home-directory environments solely because their names look temporary.

Attribute each candidate to a MAKO task using its path, creation commands/task record, environment metadata, or container labels/mounts. Check current build and test scripts, launchers, scheduled jobs, and processes for consumers. Inspect a candidate's own metadata with system tools rather than activating it or executing its Python/startup scripts. A copied interpreter's link to a shared installation does not authorize deleting the interpreter target. Do not follow symlinks out of the selected environment. Preserve user-authored experiment scripts, unique source changes, and licensed inputs; split these from generated environment directories when they coexist.

Read-only container inventory should identify stopped MAKO experiment containers, task-specific images, volumes, and writable layers. Preserve the portable release builders, Flatpak cache volumes, shared base images, regular QA runtimes, and active containers. Use runtime-reported exclusive/reclaimable sizes where available; shared layers are not additive. Containers started with automatic removal may leave no resource to clean. Remove only approved exact IDs through the owning runtime; never delete its storage directories directly or perform a global prune. A task-private Wine/Proton prefix can be disposable; installed Steam runtimes, Proton versions, game prefixes, and user game data remain protected.

Codex's bundled runtime, installed packages/plugins, and environments powering the current assistant or supported tools are not experiment leftovers. If an experiment used a remote or managed sandbox, report its host and storage separately and use its supported lifecycle interface only when available and authorized. Do not claim such a sandbox exists or contributes reclaimable local bytes without evidence.

## Preserve source and durable data

Keep `.git` stores, branches, worktrees with unique/uncommitted work, tracked assets and release artwork, documentation, config, credentials, licensed inputs such as `Lossless.dll`, installed Renderer/Decky, profiles, and game data. MAKO Traces' `traces/` and `user-reports/` are curated integrity-protected archives. Inspect Git size if useful, but do not rewrite history, expire reflogs, or prune Git objects as ordinary cleanup.

Unused source assets are a source-maintenance decision. This cleanup covers temporary generated assets with verified provenance and no remaining references. Old timestamps or a failed text search alone do not prove an asset is unused.
