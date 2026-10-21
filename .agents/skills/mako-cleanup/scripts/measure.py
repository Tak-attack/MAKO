#!/usr/bin/env python3
"""Measure an explicit cleanup plan without modifying or deleting its targets."""

import argparse
from collections import defaultdict
import hashlib
import json
import os
from pathlib import Path
import stat
import sys


def validate_entries(plan):
    entries = plan.get("entries")
    if not isinstance(entries, list) or not entries:
        raise ValueError("entries must be a nonempty list")
    normalized = []
    for entry in entries:
        if not isinstance(entry, dict):
            raise ValueError("every entry must be an object")
        if entry.get("decision") not in {"remove", "keep", "review"}:
            raise ValueError("decision must be remove, keep, or review")
        if not isinstance(entry.get("reason"), str) or not entry["reason"].strip():
            raise ValueError("every entry needs a reason")
        raw = entry.get("path")
        if not isinstance(raw, str) or not raw or "\0" in raw:
            raise ValueError("every entry needs a literal absolute path")
        path = Path(raw)
        if not path.is_absolute() or ".." in path.parts:
            raise ValueError(f"path must be absolute without '..': {raw}")
        if path.parent.resolve() != path.parent:
            raise ValueError(f"symlinked ancestor is not allowed: {raw}")
        for other in normalized:
            previous = Path(other["path"])
            if path == previous or path in previous.parents or previous in path.parents:
                raise ValueError(f"overlapping roots: {path} and {previous}; split into children")
        normalized.append({**entry, "path": str(path)})
    return sorted(normalized, key=lambda item: item["path"])


def measure(plan):
    entries = validate_entries(plan)
    rows = []
    inodes = {}
    filesystems = {}
    for index, entry in enumerate(entries):
        root = Path(entry["path"])
        row = {**entry, "allocated_bytes": 0, "estimated_reclaimable_bytes": 0,
               "nodes": 0, "errors": []}
        rows.append(row)
        digest = hashlib.sha256()
        try:
            device = root.lstat().st_dev
            usage = os.statvfs(root.parent)
            filesystems.setdefault(device, {"device": device, "sample_path": str(root.parent),
                                           "available_bytes": usage.f_bavail * usage.f_frsize})
        except OSError as error:
            row["errors"].append(str(error))
            row["fingerprint"] = None
            continue
        pending = [root]
        while pending:
            path = pending.pop()
            try:
                info = path.lstat()
                if info.st_dev != device or (stat.S_ISDIR(info.st_mode) and os.path.ismount(path)):
                    raise OSError(f"mount boundary not traversed: {path}")
                if not (stat.S_ISDIR(info.st_mode) or stat.S_ISREG(info.st_mode)
                        or stat.S_ISLNK(info.st_mode)):
                    raise OSError(f"special file requires review: {path}")
                metadata = (str(path), info.st_dev, info.st_ino, info.st_mode, info.st_size,
                            info.st_blocks, info.st_nlink, info.st_mtime_ns, info.st_ctime_ns)
                digest.update(json.dumps(metadata, ensure_ascii=True).encode() + b"\n")
                key = (info.st_dev, info.st_ino)
                if key not in inodes:
                    inodes[key] = {"bytes": info.st_blocks * 512, "links": info.st_nlink,
                                   "directory": stat.S_ISDIR(info.st_mode), "owners": []}
                    row["allocated_bytes"] += info.st_blocks * 512
                inodes[key]["owners"].append(index)
                row["nodes"] += 1
                if stat.S_ISDIR(info.st_mode):
                    with os.scandir(path) as children:
                        pending.extend(sorted((Path(child.path) for child in children), reverse=True))
            except OSError as error:
                row["errors"].append(str(error))
        row["fingerprint"] = digest.hexdigest() if not row["errors"] else None

    for inode in inodes.values():
        selected = [i for i in inode["owners"] if rows[i]["decision"] == "remove"]
        if selected and (inode["directory"] or len(selected) == inode["links"]):
            rows[selected[0]]["estimated_reclaimable_bytes"] += inode["bytes"]
    complete = all(not row["errors"] for row in rows)
    totals = defaultdict(int)
    for row in rows:
        totals[row["decision"]] += row["allocated_bytes"]
        if row["errors"]:
            row["estimated_reclaimable_bytes"] = None
    return {
        "complete": complete,
        "rows": rows,
        "allocated_bytes_by_decision": dict(totals),
        "estimated_reclaimable_bytes": sum(row["estimated_reclaimable_bytes"] for row in rows)
        if complete else None,
        "filesystems": list(filesystems.values()),
        "notes": [
            "Read-only measurement; classifications and deletion authorization come from the reviewed plan.",
            "Shared inodes are allocated to the first sorted path, once across all rows.",
            "Reclaimable bytes exclude nonselected hard links; snapshots, reflinks, compression, and open files may reduce actual savings.",
            "Fingerprints cover names and stat metadata, not contents; recheck activity and protected inputs before deletion.",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("plan", type=Path, help="JSON containing entries with path, decision, reason")
    args = parser.parse_args()
    try:
        with args.plan.open() as source:
            plan = json.load(source)
        if not isinstance(plan, dict):
            raise ValueError("plan must be a JSON object")
        result = measure(plan)
    except (OSError, ValueError, TypeError) as error:
        parser.exit(2, f"Cannot measure plan: {error}\n")
    json.dump(result, sys.stdout, indent=2)
    print()
    return 0 if result["complete"] else 1


if __name__ == "__main__":
    sys.exit(main())
