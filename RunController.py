from __future__ import annotations
import os
from pathlib import Path
import subprocess
from dataclasses import asdict, dataclass

@dataclass(frozen=True)
class RunControls:
    """Define the time range and storage cadence passed to the engine."""
    start_time_s: float = 0.0
    end_time_s: float = 0.0
    step_size_s: float = 0.0
    storage_rate: int = 1
    def as_dict(self):
        """Convert controls to metadata suitable for the run registry."""
        return asdict(self)

@dataclass(frozen=True)
class RunResult:
    """Contain the physics-engine result and any launch failure text."""
    return_code: int
    output: str
    error_text: str | None = None

class RunController:
    """Own the physics-engine process for one Backend Controls subsystem."""

    def __init__(self, physics_engine: Path | None = None):
        """Resolve the configured or default physics-engine executable."""
        self.physics_engine = physics_engine or self._find_physics_engine()
        self._process = None

    @staticmethod
    def _find_physics_engine() -> Path:
        """Find the first available engine build or return the default candidate."""
        configured = os.environ.get("PHYSICS_ENGINE_EXECUTABLE")
        if configured: return Path(configured)
        root = Path(__file__).resolve().parent
        candidates = (root / "mlss" / "x64" / "Debug" / "mlss.exe", root / "mlss" / "x64" / "Release" / "mlss.exe", root / "mlss.exe")
        return next((candidate for candidate in candidates if candidate.exists()), candidates[0])

    def start_run(self, d01_json: str, controls: RunControls, run_path: Path | None = None) -> RunResult:
        """Start the engine, send unchanged D-01, and capture its output."""
        if not self.physics_engine.exists(): return RunResult(-1, "", f"could not find physics engine {self.physics_engine}")
        args = ["--run", str(controls.start_time_s), str(controls.end_time_s), str(controls.step_size_s), str(controls.storage_rate)]
        # communicate() writes D-01 once, then waits for the engine to finish.
        try:
            self._process = subprocess.Popen([str(self.physics_engine), *args], cwd=run_path, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            output, _ = self._process.communicate(d01_json)
        except OSError as error:
            return RunResult(-1, "", f"could not start physics engine: {error}")
        finally:
            self._process = None
        if run_path is not None:
            (run_path / "D-01.json").write_text(d01_json, encoding="utf-8")
            (run_path / "engine-output.log").write_text(output, encoding="utf-8")
        return RunResult(self._parse_result(output), output)

    def stop_run(self):
        """Terminate the active physics-engine process, if one exists."""
        if self._process is not None: self._process.terminate()

    @staticmethod
    def _parse_result(output: str) -> int:
        """Extract the final RUN_RESULT status emitted by the engine."""
        for line in reversed(output.splitlines()):
            if line.startswith("RUN_RESULT "):
                try: return int(line.removeprefix("RUN_RESULT "))
                except ValueError: pass
        return -1
