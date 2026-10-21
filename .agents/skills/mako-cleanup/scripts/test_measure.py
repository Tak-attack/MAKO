"""Synthetic checks for space accounting; all fixtures live in temporary trees."""

import importlib.util
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("measure", Path(__file__).with_name("measure.py"))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class MeasureTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def file(self, name):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"x" * 8192)
        return path

    def plan(self, *entries):
        return {"entries": [{"path": str(path), "decision": decision, "reason": "fixture"}
                            for path, decision in entries]}

    def test_kept_cache_is_not_counted_as_savings_and_targets_are_unchanged(self):
        cache = self.file("build/cache/sdk")
        output = self.file("build/output")
        before = {p: (p.read_bytes(), p.stat().st_mtime_ns) for p in (cache, output)}
        result = module.measure(self.plan((cache.parent, "keep"), (output, "remove")))
        self.assertTrue(result["complete"])
        self.assertEqual(result["estimated_reclaimable_bytes"], output.stat().st_blocks * 512)
        self.assertEqual(before, {p: (p.read_bytes(), p.stat().st_mtime_ns) for p in (cache, output)})

    def test_review_is_excluded(self):
        output = self.file("uncertain")
        self.assertEqual(module.measure(self.plan((output, "review")))["estimated_reclaimable_bytes"], 0)

    def test_overlap_and_duplicate_paths_are_rejected(self):
        output = self.file("build/cache/sdk")
        for entries in (((output.parent.parent, "remove"), (output.parent, "keep")),
                        ((output, "remove"), (output, "keep"))):
            with self.assertRaises(ValueError):
                module.measure(self.plan(*entries))

    def test_symlinks_are_not_followed(self):
        target = self.file("outside/important")
        link = self.root / "output-link"
        link.symlink_to(target.parent, target_is_directory=True)
        result = module.measure(self.plan((link, "remove")))
        self.assertEqual(result["rows"][0]["nodes"], 1)
        self.assertEqual(result["estimated_reclaimable_bytes"], link.lstat().st_blocks * 512)
        with self.assertRaises(ValueError):
            module.measure(self.plan((link / "important", "remove")))

    def test_hardlinks_are_counted_once_and_only_freed_if_all_selected(self):
        first = self.file("a")
        second = self.root / "b"
        os.link(first, second)
        allocated = first.stat().st_blocks * 512
        for plan in (self.plan((first, "remove")), self.plan((first, "remove"), (second, "keep"))):
            self.assertEqual(module.measure(plan)["estimated_reclaimable_bytes"], 0)
        result = module.measure(self.plan((first, "remove"), (second, "remove")))
        self.assertEqual(result["estimated_reclaimable_bytes"], allocated)
        self.assertEqual(sum(row["allocated_bytes"] for row in result["rows"]), allocated)

    def test_missing_paths_are_incomplete(self):
        result = module.measure(self.plan((self.root / "missing", "remove")))
        self.assertFalse(result["complete"])
        self.assertIsNone(result["estimated_reclaimable_bytes"])
        self.assertTrue(result["rows"][0]["errors"])

    def test_mount_boundaries_are_not_walked(self):
        output = self.file("mount/output")
        with patch.object(module.os.path, "ismount", side_effect=lambda p: p == output.parent):
            result = module.measure(self.plan((output.parent, "remove")))
        self.assertFalse(result["complete"])
        self.assertEqual(result["rows"][0]["nodes"], 0)

    def test_fingerprint_changes_when_contents_metadata_changes(self):
        output = self.file("output")
        plan = self.plan((output, "remove"))
        before = module.measure(plan)["rows"][0]["fingerprint"]
        output.write_bytes(b"replacement")
        self.assertNotEqual(before, module.measure(plan)["rows"][0]["fingerprint"])

    def test_sparse_file_uses_allocated_blocks(self):
        output = self.root / "sparse"
        with output.open("wb") as stream:
            stream.seek(8 * 1024 * 1024)
            stream.write(b"x")
        result = module.measure(self.plan((output, "remove")))
        self.assertEqual(result["estimated_reclaimable_bytes"], output.stat().st_blocks * 512)
        self.assertLess(result["estimated_reclaimable_bytes"], output.stat().st_size)


if __name__ == "__main__":
    unittest.main()
