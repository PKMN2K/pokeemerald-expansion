#!/usr/bin/env python3
"""Exercise CI download recovery without network access or root package changes."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

INSTALLER = Path(__file__).with_name("install-dependencies.sh")
MOCK = r'''#!/usr/bin/env python3
import json, os, pathlib, sys, time
command = pathlib.Path(sys.argv[0]).name
args = sys.argv[1:]
if command == "sudo":
    os.execvp(args[0], args)
if command == "timeout":
    # Exercise a real timeout, shortened only for the simulated stalled fetch.
    assert args[:2] == ["--verbose", "--kill-after=15s"]
    assert args[2] in ("90s", "180s")
    os.execv("/usr/bin/timeout", ["timeout", "--kill-after=1s", "1s", *args[3:]])
log = pathlib.Path(os.environ["MOCK_LOG"])
previous = [json.loads(line) for line in log.read_text().splitlines()] if log.exists() else []
stage = "update" if "update" in args else "download" if "--download-only" in args else "install"
source = next(arg.split("=", 1)[1] for arg in args if arg.startswith("Dir::Etc::sourcelist="))
sources = pathlib.Path(source).read_text()
with log.open("a") as f:
    f.write(json.dumps(dict(stage=stage, args=args, sources=sources)) + "\n")
attempt = sum(row["stage"] == stage for row in previous) + 1
case = os.environ["MOCK_CASE"]
if case == "stalled_download" and stage == "download" and attempt == 1:
    time.sleep(5)
if case == "failed_metadata" and stage == "update" and attempt == 1:
    sys.exit(100)
if case == "both_failed" and stage == "download":
    sys.exit(100)
if case == "failed_install" and stage == "install":
    sys.exit(100)
'''


class InstallerRecovery(unittest.TestCase):
    def exercise(self, case):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            mock = root / "mock"
            mock.write_text(MOCK)
            mock.chmod(0o755)
            for command in ("sudo", "timeout", "apt-get"):
                (root / command).symlink_to(mock)
            log = root / "calls.jsonl"
            env = dict(os.environ, PATH=f"{root}:{os.environ['PATH']}",
                       MOCK_LOG=str(log), MOCK_CASE=case)
            result = subprocess.run(["bash", str(INSTALLER), "python3-pil", "libmgba-dev"],
                                    env=env, text=True, capture_output=True, timeout=15)
            calls = [json.loads(line) for line in log.read_text().splitlines()]
            self.assertIn("https://archive.ubuntu.com/ubuntu", calls[0]["sources"])
            for call in calls:
                self.assertIn("signed-by=/usr/share/keyrings/ubuntu-archive-keyring.gpg", call["sources"])
                if call["stage"] != "update":
                    self.assertIn("libmgba-dev", call["args"])
                    self.assertIn("python3-pil", call["args"])
            return result, calls

    def test_primary_success(self):
        result, calls = self.exercise("success")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual([c["stage"] for c in calls], ["update", "download", "install"])
        self.assertIn("--no-download", calls[-1]["args"])

    def test_fallback_after_stall_or_metadata_failure(self):
        for case in ("stalled_download", "failed_metadata"):
            with self.subTest(case=case):
                result, calls = self.exercise(case)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(sum(c["stage"] == "install" for c in calls), 1)
                self.assertIn("azure.archive.ubuntu.com/ubuntu", calls[-1]["sources"])

    def test_both_mirrors_fail_without_installing(self):
        result, calls = self.exercise("both_failed")
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("install", [c["stage"] for c in calls])

    def test_install_failure_is_not_hidden_or_retried(self):
        result, calls = self.exercise("failed_install")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual([c["stage"] for c in calls], ["update", "download", "install"])


if __name__ == "__main__":
    unittest.main()
