"""Backend-controls command interface for the C++ physics engine."""

from __future__ import annotations

import os
from pathlib import Path
import shlex
import subprocess
import sys
from dataclasses import dataclass


@dataclass
class RunControls:
	start_time_s: float = 0.0
	end_time_s: float = 0.0
	step_size_s: float = 0.0
	storage_rate: int = 1


class RunController:
	def __init__(self, physics_engine: Path | None = None) -> None:
		self.config_json = ""
		self.controls = RunControls()
		self.config_loaded = False
		self.controls_set = False
		self.physics_engine = physics_engine or self._find_physics_engine()
		self._process: subprocess.Popen[str] | None = None

	@staticmethod
	def _find_physics_engine() -> Path:
		configured = os.environ.get("PHYSICS_ENGINE_EXECUTABLE")
		if configured:
			return Path(configured)

		root = Path(__file__).resolve().parent
		candidates = (
			root / "mlss" / "x64" / "Debug" / "mlss.exe",
			root / "mlss" / "x64" / "Release" / "mlss.exe",
			root / "mlss" / "Debug" / "mlss.exe",
			root / "mlss" / "Release" / "mlss.exe",
			root / "mlss.exe",
		)
		for candidate in candidates:
			if candidate.exists():
				return candidate
		return candidates[0]

	def command_loop(self) -> None:
		print("[Python backend controls] RunController.py active")
		self.print_help()
		print("> ", end="", flush=True)
		for line in sys.stdin:
			if not self.handle_command(line.rstrip("\n")):
				break
			print("> ", end="", flush=True)

	def handle_command(self, line: str) -> bool:
		try:
			parts = shlex.split(line)
		except ValueError as error:
			print(f"invalid command: {error}")
			return True

		if not parts:
			return True

		command = parts[0]
		if command == "help":
			self.print_help()
		elif command == "load":
			if len(parts) != 2:
				print("usage: load <file>")
			elif self.load_config(parts[1]):
				print(f"D-01 loaded ({len(self.config_json.encode('utf-8'))} bytes)")
		elif command == "settings":
			self.set_settings(parts[1:])
		elif command == "status":
			self.print_status()
		elif command == "run":
			result = self.start_processing()
			messages = {
				0: "run complete",
				-1: "run not started: not ready, or configuration rejected (-1)",
				-2: "run aborted by the health monitor (-2)",
			}
			print(messages.get(result, f"run ended with code {result}"))
		elif command in {"quit", "exit"}:
			return False
		else:
			print(f"unknown command '{command}' (type help)")
		return True

	def load_config(self, path: str) -> bool:
		requested_path = Path(path).expanduser()
		candidates = [requested_path]
		if not requested_path.is_absolute():
			candidates.append(Path(__file__).resolve().parent / requested_path)

		for candidate in candidates:
			try:
				self.config_json = candidate.read_text(encoding="utf-8")
				self.config_loaded = True
				return True
			except OSError:
				continue

		print(f"could not open '{path}'")
		return False

	def set_settings(self, values: list[str]) -> None:
		if len(values) != 4:
			print("usage: settings <start> <end> <step> <rate>   (times in seconds; rate = store every Nth step)")
			return
		try:
			controls = RunControls(float(values[0]), float(values[1]), float(values[2]), int(values[3]))
		except ValueError:
			print("usage: settings <start> <end> <step> <rate>   (times in seconds; rate = store every Nth step)")
			return
		if controls.step_size_s <= 0.0 or controls.end_time_s <= controls.start_time_s or controls.storage_rate < 1:
			print("invalid: need step > 0, end > start, rate >= 1")
			return
		self.controls = controls
		self.controls_set = True
		print("run controls set")

	def start_processing(self) -> int:
		if not self.config_loaded or not self.controls_set:
			print("load a D-01 file and set the run settings first (type status)")
			return -1

		if not self.physics_engine.exists():
			print(f"could not find physics engine '{self.physics_engine}'")
			return -1

		c = self.controls
		arguments = ["--run", str(c.start_time_s), str(c.end_time_s), str(c.step_size_s), str(c.storage_rate)]
		try:
			self._process = subprocess.Popen(
				[str(self.physics_engine), *arguments],
				stdin=subprocess.PIPE,
				stdout=subprocess.PIPE,
				stderr=subprocess.STDOUT,
				text=True,
			)
			output, _ = self._process.communicate(self.config_json)
		except OSError as error:
			print(f"could not start physics engine: {error}")
			return -1
		finally:
			self._process = None

		print(output, end="")
		for line in reversed(output.splitlines()):
			if line.startswith("RUN_RESULT "):
				try:
					return int(line.removeprefix("RUN_RESULT "))
				except ValueError:
					break
		return -1

	def stop_processing(self) -> None:
		if self._process is not None:
			self._process.terminate()

	@staticmethod
	def print_help() -> None:
		print("Run Controller (physics engine test harness)")
		print("  help                                   this list")
		print("  load <file>                            read D-01 (JSON text) from a file")
		print("  settings <start> <end> <step> <rate>   run controls: times in seconds, rate = store every Nth step")
		print("  flight generation is configured in scenario.json under the aircraft object")
		print("  status                                 show what is loaded / set")
		print("  run                                    fire the start signal")
		print("  quit                                   exit")

	def print_status(self) -> None:
		print(f"D-01: {'loaded' if self.config_loaded else 'not loaded'}")
		if self.controls_set:
			c = self.controls
			print(f"run controls: start {c.start_time_s} s, end {c.end_time_s} s, step {c.step_size_s} s, store every {c.storage_rate} step(s)")
		else:
			print("run controls: not set")
		print("aircraft generation: configured in scenario.json")


if __name__ == "__main__":
	RunController().command_loop()
