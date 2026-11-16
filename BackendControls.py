"""Backend Controls subsystem entry point."""

from __future__ import annotations

from pathlib import Path
import subprocess
import sys


def main() -> int:
	root = Path(__file__).resolve().parent
	run_controller = root / "RunController.py"
	print("[BackendControls] starting RunController.py", flush=True)
	return subprocess.run([sys.executable, str(run_controller)], cwd=root).returncode


if __name__ == "__main__":
	raise SystemExit(main())
