# Codex chats and assets

Resolve storage from `CODEX_HOME`, falling back to `~/.codex`. Inspect only MAKO-owned data. Preserve config, credentials, skills, plugins, runtime packages, automations, active sessions, shared databases, and unrelated projects. Never bulk-delete Codex home or shared asset directories.

## Attribute and inventory

Prefer app tools for projects, threads, archived threads, and artifacts; paginate archived results fully. Match canonical project roots under the shared MAKO directory or a worktree's verified Git common directory/remote, not the word “MAKO” in a title. A projectless chat needs corroborating metadata or user identification. Use exact source-provided titles in the report.

Inspect `archived_sessions` and session metadata as needed to locate/measure rollouts. A JSONL file's first `session_meta` record identifies its `cwd` and thread ID, but filenames and fork metadata alone do not prove current archive status. Confirm against the app inventory. If necessary, inspect the local state schema and select only identity/path/archive/project fields through a read-only database connection. Database versions are not stable; do not hardcode one or read message bodies/authentication fields. Never alter database records manually.

Inventory `generated_images`, `visualizations`, and `attachments` by owning thread ID; include linked projectless output directories only after verifying ownership. Preserve the current thread and its assets. Archived status does not mean an asset is unused: check retained chats, attached artifacts, source/release artwork references, and ongoing work. If only an original exists, classify it for review rather than calling it a duplicate. Broad directory totals are not automatically reclaimable savings.

Include task-private virtual environments and experiment sandboxes referenced by completed MAKO threads, even when located outside the repository or Codex home. Follow [temporary experiment environments](storage-map.md#temporary-experiment-environments) to distinguish disposable installations from Codex's own bundled runtime and regular MAKO build/test tooling. Collect their paths and ownership evidence before deleting the chats that identify them.

For managed worktrees, inspect attachments, status, untracked/ignored inputs, unique commits, and active consumers. Use supported lifecycle tools only for worktrees they can manage. Account for retained recovery snapshots. Do not bypass ownership restrictions by removing checkout directories directly. Archiving the current chat or deleting the current checkout is outside this skill.

## Delete after approval

The official [Codex app-server documentation](https://learn.chatgpt.com/docs/app-server#delete-a-thread) documents `thread/delete` with `params: {"threadId": "..."}`. It removes persisted rollouts and metadata **and spawned descendants**. Include the full descendant set in the report and approval. Protect running, current, pinned, unrelated, or still-needed descendants; do not delete their ancestor if any would be affected. A loaded-thread query on a new connection does not prove inactivity on the desktop's existing server.

Use an available supported delete tool/app UI or a documented app-server connection supported by the installed version. Inspect current capabilities/schema/help first. If permanent deletion is unavailable, mark the rows unavailable and exclude them from actionable savings. Do not improvise SQLite deletes, remove rollouts behind the app's back, or start a second writer against the same store without documented support. Complete independently approved repository cleanup and report unresolved chat cleanup plainly.

`thread/archive` and `set_thread_archived` retain conversation data. They do not implement permanent deletion or free the rollout's space. Official reference: [archive semantics](https://learn.chatgpt.com/docs/app-server#archive-a-thread).

After supported deletion, verify the targeted IDs and rollouts are gone. Do not assume attachments, images, visualizations, snapshots, or SQLite files also shrank. Re-inventory residual files and remove only separately approved, now-unreferenced paths. Never vacuum/remove live shared databases, WAL files, locks, or global logs to pursue extra savings. Measure filesystem changes and report partial outcomes accurately.
