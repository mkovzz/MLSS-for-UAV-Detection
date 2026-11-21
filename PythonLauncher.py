"""Top-level launcher for the Python subsystem hierarchy."""

from __future__ import annotations

from pathlib import Path
import subprocess
import sys


def main() -> int:
	"""Launch the Backend Controls subsystem in the repository root."""
	root = Path(__file__).resolve().parent
	backend_controls = root / "BackendControls.py"
	print("[PythonLauncher] starting BackendControls.py", flush=True)
	return subprocess.run([sys.executable, str(backend_controls)], cwd=root).returncode


if __name__ == "__main__":
	raise SystemExit(main())
