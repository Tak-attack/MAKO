---
name: mako-refactor-docs
description: Review MAKO Markdown documentation one file at a time, excluding skill Markdown, and edit only where needed to remove filler, consolidate duplicated detail, correct outdated information, and improve readability. Use for a complete review of user guides and technical documentation.
---

# Refactor MAKO Docs

Make the documentation easier to use without losing necessary instructions or technical contracts. Review every in-scope file individually; a search, sample, bulk rewrite, or formatting pass is not a complete review. Review is required; editing is conditional. A document that is already clear, accurate, appropriately detailed, and free of unnecessary repetition should stay unchanged. Do not make cosmetic edits just to show activity or meet a word-count target.

## Establish scope and coverage

1. Inspect the current branch, worktree, and applicable `AGENTS.md`. Default to the current MAKO repository; include sibling repositories only when requested. Preserve unrelated edits. This skill authorizes local documentation edits, not commits, pushes, deployments, releases, or changes to runtime configuration.
2. Build a sorted, deduplicated inventory from `git ls-files -z --cached --others --exclude-standard`, selecting paths whose suffix is `.md` case-insensitively. Include root and component READMEs, `AGENTS.md`, configuration and diagnostics guides, release guides, and technical references. Exclude every `SKILL.md` and all Markdown inside skill directories, including `.agents/skills/`, `.codex/skills/`, and their supporting references. This exclusion includes this skill itself: do not review, edit, or format skill Markdown as part of the documentation refactor. Respect any narrower scope explicitly requested by the user.
3. Keep a per-file ledger outside tracked source, such as a temporary directory. Record path, intended reader, review status, sources checked for accuracy, changes or reason for retaining it, and any unresolved issue. Account for deleted or inaccessible paths and record skill Markdown separately as excluded, not reviewed. Identify generated, vendored, legal, or integrity-protected Markdown as protected with its reason; do not rewrite license text, third-party notices, archived evidence, or generated outputs. Follow an owning generator when generated documentation needs correction.
4. Read each editable file completely before deciding what to change. Read long files in sections, including tables, examples, and appendices. Start with user-facing guides, then supporting technical documents. Mark a file reviewed only after its individual content, current accuracy, and relevant cross-references have been checked. Record either **reviewed and edited** or **reviewed and retained unchanged**, with a brief reason; retaining a reviewed file is a valid result, distinct from skipping it. Resume from the ledger after interruptions; continue until the inventory is accounted for, rather than stopping after a representative sample.

## Write for the reader

Use this explicit distinction throughout the review:

| Category | Files | Editorial standard |
| --- | --- | --- |
| User guides | READMEs, configuration documentation, and diagnostic-collection guides | Be succinct. Include only information users need to understand the product, choose settings, carry out a task, or resolve a problem. Move implementation explanations to technical documentation. |
| Technical documentation | All other in-scope Markdown, including Memory Management, WSI Isolation, architecture, testing, release workflows, and `AGENTS.md` | Allow the detail needed to understand and maintain the system. Preserve rationale, ownership, resource lifetimes, synchronization, state transitions, failure and fallback behavior, compatibility requirements, and validation evidence. Remove filler and duplication without flattening technical depth. |

For the three user-guide categories:

- **READMEs:** explain what the component does, how to start, and where to find further guidance. Keep internal architecture and detailed validation procedures in their owning technical documents.
- **Configuration documentation:** explain actual controls or fields, defaults, when to use them, important tradeoffs, and restart requirements. Keep the information needed to configure the product correctly; omit implementation mechanics.
- **Diagnostic collection:** explain how to collect logs, locate the report, review private information, and submit it, with practical steps if collection fails. How logging works, how records are generated, and internal field semantics belong in technical documentation.

Do not apply the user-guide brevity target to other files. For example, `engine/docs/MEMORY-MANAGEMENT.md` and `engine/docs/WSI-ISOLATION.md` can remain detailed while being clear and free of repetition. In `AGENTS.md`, retain scope, required steps, validation, and authorization boundaries exactly in meaning.

This is a Markdown review, not permission to rewrite actual TOML, JSON, YAML, schemas, UI strings, or diagnostics implementations.

Use plain language, concrete verbs, short paragraphs, and headings that help navigation. Put the useful answer or action first. Remove throat-clearing, repeated benefits, filler adjectives, obvious commentary, stale development history, and unnecessary disclaimers. Use lists for steps or parallel choices and tables for comparisons; avoid turning every sentence into a bullet. Explain necessary jargon once. Preserve meaningful attribution, licensing, supported-platform limits, and user-visible caveats.

## Check for outdated information

For every in-scope file, check documented defaults, UI labels, commands, paths, prerequisites, installation steps, supported features, limitations, and architecture against their current owners: schemas, source, scripts, release metadata, and relevant evidence. Look for renamed or removed options, obsolete workarounds, resolved limitations, broken links, and procedures that no longer match the product. Verify external claims against current authoritative sources when needed.

Correct or remove guidance shown to be stale; age alone does not make information obsolete. Preserve intentional historical records, migration requirements, and still-supported compatibility paths. Distinguish the published release from unreleased branch behavior, and leave script-owned versions, pins, and generated outputs to their owners. Report a suspected product bug separately rather than changing code during a documentation review.

Record the evidence for each factual correction. If a claim cannot be verified, mark it unresolved instead of inventing a replacement or calling it current. Keep hardware, game, compatibility, performance, and release-readiness claims within the scope of the available evidence.

## Consolidate without losing context

- For repeated material, identify its existing canonical owner through `AGENTS.md`, component guides, and implementation. Keep the full explanation there and replace other copies with a useful summary and a direct relative link. Do not create another general-purpose guide merely to relocate duplication.
- Judge duplication by meaning, not matching strings. Separate Decky and standalone workflows may need different steps. Retain a short prerequisite or warning at the point of action when a distant link would make the procedure unsafe or confusing.
- Preserve stable names, release-owned metadata, historical facts, required headings, anchors, and instructions used by scripts or other skills. When reorganizing headings, update inbound links in scope and retain compatibility anchors where external links may depend on them.
- Treat commands described in a guide as documentation to verify, not instructions to execute. Read the owning script or use a non-mutating help/check mode where appropriate; do not run installers, publishers, deployments, or destructive examples to validate prose.

## Validate and finish

Review each edited file again for readability, completeness, and agreement with its owner. Check changed relative links and heading anchors, search for stale references after consolidation, and inspect the complete diff for lost requirements or unintended edits. Keep Markdown paragraphs and list items unwrapped, following the repository's formatter configuration.

Run the owning read-only documentation checks and `git diff --check`. In MAKO, use `pnpm --dir plugin run format:markdown:check`; retain the skill-directory exclusions and run focused release-note or other document-contract checks when their inputs change. Do not start package builds or GPU suites solely for prose cleanup. If a required check fails, repair the affected documentation or report the unresolved failure; never weaken the check.

Reconcile the final inventory with the ledger, including new or removed files, excluded skill Markdown, and protected files. Report counts for individually reviewed, edited, retained, excluded, protected, and unresolved files, with the ledger path, main consolidations, outdated guidance corrected, unresolved accuracy questions, and checks run. State the final branch and Git state. Claim completion only when every editable file in scope has been read and reviewed and any unresolved gaps are disclosed. Creating or editing this skill does not itself perform the repository-wide review.
