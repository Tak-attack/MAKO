---
name: mako-cleanup
description: Analyze and reclaim disposable MAKO storage after releases across the repositories under ~/mako and related Codex archived chats, worktrees, generated assets, and temporary test or experiment environments. Report estimated savings and what will be kept, then require confirmation before deletion. Preserve Flatpak caches and regularly used build and release tooling.
---

# MAKO cleanup

Free space from completed builds, packages, investigations, and unused Codex material while retaining the inputs and tools needed for future builds and releases. The usual use is after a release: **there is no age cutoff; recent completed output is eligible too. Show the concrete cleanup report and wait for the user's explicit confirmation before deleting anything.** A report-only request ends after the report.

Cleaning test leftovers is a primary purpose of this skill. Explicitly inspect completed, failed, and abandoned test runs, generated test binaries and sanitizer builds, coverage, logs, screenshots, rendered comparisons, reports, temporary fixtures, and disposable test environments. Include recent results too. Preserve test source, canonical fixtures/baselines, shared tooling and caches, and files still in use; do not retain completed generated results merely because they are test evidence.

## Discover and classify

1. Locate the shared MAKO directory: normally `/home/deck/mako`, otherwise `~/mako` or the parent of the current MAKO checkout. Discover its actual repositories, including vkBasalt and historical or experimental checkouts. Inspect each repository's branch, worktree, `AGENTS.md`, `.gitignore`, and owning build/storage guides. Preserve source and local changes. Do not switch branches, commit, push, or delete whole repositories.
2. Read [the storage map](references/storage-map.md). Trace current build scripts and path overrides before classifying storage. Build directories can contain reusable SDKs, Flatpak data, and dependency downloads. Do not treat `build`, `out`, `assets`, `cache`, or Git-ignored status alone as proof of disposability.
3. Measure disk use by filesystem and drill into the largest relevant directories. Use read-only `du -x`/`df`, Git inspection, and container inventory. Use host execution permissions when needed to inspect the actual container store; report inaccessible storage as unmeasured. Do not install tools, build projects, download packages, or clean anything during discovery.
4. Inspect Codex storage using [the Codex procedure](references/codex-cleanup.md). Establish MAKO ownership from project paths, repository identity, and thread IDs. Protect the current thread, running or otherwise in-use tasks, and unrelated chats. Inspect metadata first; do not dump transcripts or credentials into the report.
5. Inspect [temporary experiment environments](references/storage-map.md#temporary-experiment-environments), including Python virtual environments, one-off Gym/release sandboxes, temporary prefixes, and stopped experiment containers. Follow verified MAKO/Codex task references outside the repositories into temporary or task-owned directories. Environments made only for completed experiments are eligible for removal, including their private dependencies; being a virtual environment does not automatically make one protected build tooling.
6. Build an exact-path plan: `remove` for verified disposable material, `keep` for protected/reusable material, and `review` for uncertain ownership or unavailable deletion mechanisms. Resolve uncertainty independently where possible. Include eligible archived chats, generated assets, and disposable environments, not just repository builds.

Propose removing all verified disposable output, including recent completed MAKO Gym runs and post-release candidates. Do not retain extra copies merely because they are recent or once supported a release. Preserve candidates or inputs still actively needed by a build, release, or investigation, and explain why. Disclose that deleting Gym results loses those local historical measurements. Curated MAKO Traces archives remain protected. Removing incremental build trees causes recompilation next time; retained dependency caches avoid unnecessary downloads.

## Measure and report

Write a temporary JSON plan outside every deletion target. Each entry has an absolute `path`, `decision` (`remove`, `keep`, or `review`), and `reason`. Optional labels such as `repository` and `thread_id` may be included. Split mixed trees into explicit children: list `engine/build/cache` as `keep` and verified disposable siblings separately. Never propose deleting a parent with a protected child.

For managed environments or container resources, also record the owning manager/runtime, host, exact resource ID or environment prefix, and its targeted removal operation. Measure these through the owner's read-only inventory and account for shared layers separately; the filesystem helper only measures literal paths. Do not count a container's bind-mounted directory both as repository data and container storage, or remote disk savings as local free space.

```json
{
  "entries": [
    {"path": "/absolute/MAKO/engine/build/cache", "decision": "keep", "reason": "Reusable build dependencies"},
    {"path": "/absolute/MAKO/plugin/out/tester.zip", "decision": "remove", "reason": "Completed release's tester package"}
  ]
}
```

Run the bundled read-only helper with resolved paths:

```bash
python3 /path/to/mako-cleanup/scripts/measure.py /tmp/mako-cleanup-plan.json > /tmp/mako-cleanup-measurement.json
```

The helper has no deletion mode. It rejects overlapping roots and symlinked path ancestors, does not follow symlinks or descend into mounts, counts allocated blocks once per inode, and excludes hard-linked data from reclaimable bytes unless all links are selected for removal. It reports errors and metadata fingerprints for rechecking. It measures a plan; it does not decide whether deletion is safe. Estimates do not account for snapshots, reflink sharing, compression, or files held open by processes. Do not include incomplete rows or `review` entries in promised savings.

Present a concise report containing:

- Estimated reclaimable GiB, per-repository/category subtotals, and exact paths or a linked complete manifest. Identify storage that was not measured.
- A dedicated test-cleanup subtotal covering MAKO Gym and each repository's generated test output and temporary environments. Identify what was inspected, what is kept, and any test-storage area that could not be checked; do not double-count environments in the overall total.
- A separate Codex subtotal with exact chat titles/IDs and the deletion scope, including descendants. State that deletion is permanent.
- A disposable-environment subtotal with exact paths or resource IDs, the completed MAKO task that created each, and why it is no longer used by regular builds or tests. Identify shared environments retained for future work.
- What is kept and why, with measured sizes where practical: Flatpak caches/SDKs, toolchains, dependency caches, active work, source, installed MAKO, and durable evidence.
- Unresolved opportunities and sizes, excluded from the proposed total; explain why and what removal would lose.
- Expected rebuild cost, estimation limits, and free space on affected filesystems before cleanup.

Ask whether to proceed with that exact removal set. Explain that the pause implements the user's explicit report-before-deletion requirement and link this `SKILL.md`. Silence, a timeout, or the initial invocation is not confirmation. Keep the report and plan for the continuation; never run deletion in the same tool call as the question.

## Clean the approved set

1. Match confirmation to the reported paths and thread IDs. Excluding items needs no new approval. Adding items or changing a deletion's consequences requires an updated report and confirmation for that change.
2. Recheck Git state, process/container activity, thread status, path identities, nested repositories, mounts, protected descendants, and references. Re-run measurement and compare fingerprints for proposed removals. Skip changed or newly active items and report them; do not expand the plan or kill builds to free space. Check proposed directories for untracked source, licensed inputs, SDKs, or dependencies missed during classification.
3. Delete only approved, revalidated paths with literal arguments or argument arrays, without following symlinks or crossing mounts. For managed environments and stopped experiment containers, use the owner's exact environment prefix/resource ID and verify attached volumes separately. Use an owning cleanup command only if its exact scope matches the plan. Do not use broad globs, `git clean -fdx`, resets, global Docker/Podman pruning, global environment/package-cache purges, Flatpak pruning, or MAKO cache-prune scripts' `--confirm` modes. Never delete all of `engine/build`. Ordinary execution permissions still apply.
4. Delete approved Codex threads through a supported interface, following the Codex procedure. Verify cascades stay inside the approved set. Remove unused associated files only when separately inventoried and approved. Archiving a chat is not deletion, and worktree archival may retain a recovery snapshot; count only measured reclaimed bytes.
5. Re-measure affected filesystems, verify protected caches/tooling and Git source state remain intact, and report deletions, skipped/failed items, observed free-space change, and remaining opportunities. Concurrent writes can distort `df`; distinguish observed change from sizes removed. Do not rebuild deleted output to test cleanup. Keep the small report and manifest, not gigabytes of backup copies.

Finish with the exact branch and staged/committed/pushed state of repositories involved. This skill does not deploy or change installed MAKO components.
