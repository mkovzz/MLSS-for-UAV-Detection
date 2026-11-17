"""Terminal Command Interface for Backend Controls."""

from __future__ import annotations

import shlex
from pathlib import Path

from ConfigValidator import ConfigValidator
from RunController import RunControls, RunController
from RunRegistry import RunRegistry
from StatusPublisher import StatusPublisher


class CommandInterface:
	"""Convert terminal commands into Backend Controls component requests."""

	def __init__(
		self,
		root: Path,
		config_validator: ConfigValidator,
		run_registry: RunRegistry,
		run_controller: RunController,
		status_publisher: StatusPublisher,
	) -> None:
		"""Store references to the components owned by ``BackendControls``."""
		self.root = root or Path(__file__).resolve().parent
		self.validator = config_validator
		self.registry = run_registry
		self.controller = run_controller
		self.status_publisher = status_publisher
		self.d01_json: str | None = None
		self.controls: RunControls | None = None

	def command_loop(self) -> None:
		"""Read and dispatch commands until the user exits."""
		print("[BackendControls] Command Interface active", flush=True)
		self.print_help()
		for line in iter(input, ""):
			if not self.handle_command(line):
				break

	def handle_command(self, line: str) -> bool:
		"""Parse one command and return false when shutdown is requested."""
		# shlex preserves quoted paths while still providing shell-like command parsing.
		try:
			parts = shlex.split(line)
		except ValueError as error:
			print(f"invalid command: {error}")
			return True
		if not parts:
			return True
		command = parts[0].lower()
		if command == "help": self.print_help()
		elif command == "load": self.load_config(parts[1] if len(parts) == 2 else "")
		elif command == "settings": self.set_settings(parts[1:])
		elif command == "status": self.publish_status()
		elif command == "run": self.start_run()
		elif command in {"quit", "exit"}: return False
		else: print(f"unknown command '{command}' (type help)")
		return True

	def load_config(self, value: str) -> bool:
		"""Load D-01 text without transforming its JSON representation."""
		if not value:
			print("usage: load <file>")
			return False
		requested = Path(value).expanduser()
		candidates = [requested] if requested.is_absolute() else [requested, self.root / requested]
		for candidate in candidates:
			try:
				self.d01_json = candidate.read_text(encoding="utf-8")
				print(f"D-01 loaded ({len(self.d01_json.encode('utf-8'))} bytes)")
				return True
			except OSError:
				pass
		print(f"could not open '{value}'")
		return False

	def set_settings(self, values: list[str]) -> bool:
		"""Parse and validate run-control values supplied by the user."""
		if len(values) != 4:
			print("usage: settings <start> <end> <step> <rate>")
			return False
		try:
			controls = RunControls(float(values[0]), float(values[1]), float(values[2]), int(values[3]))
		except ValueError:
			print("invalid numeric run controls")
			return False
		if controls.step_size_s <= 0 or controls.end_time_s <= controls.start_time_s or controls.storage_rate < 1:
			print("invalid: need step > 0, end > start, rate >= 1")
			return False
		self.controls = controls
		print("run controls set")
		return True

	def start_run(self) -> int:
		"""Validate D-01, register a run, execute it, and publish its status."""
		if self.d01_json is None or self.controls is None:
			print("load a D-01 file and set run settings first")
			return -1
		validation = self.validator.validate(self.d01_json)
		if not validation.is_valid:
			messages = "; ".join(f"{error.path}: {error.message}" for error in validation.errors)
			self.status_publisher.publish(state="rejected", error_text=messages, run_list=self.registry.list_runs())
			return -1
		run_id, run_path = self.registry.create_run(self.controls.as_dict())
		self.registry.update_run(run_id, state="running")
		self.status_publisher.publish(state="running", progress=0.0, run_id=run_id, run_list=self.registry.list_runs())
		result = self.controller.start_run(self.d01_json, self.controls, run_path)
		if result.return_code == 0:
			state, error = "complete", None
		else:
			state, error = "failed", result.error_text or f"physics engine returned {result.return_code}"
		self.registry.update_run(run_id, state=state, returnCode=result.return_code)
		self.status_publisher.publish(state=state, progress=1.0 if state == "complete" else 0.0, error_text=error, run_id=run_id, run_list=self.registry.list_runs())
		if result.output: print(result.output, end="")
		return result.return_code

	def publish_status(self) -> None:
		"""Publish the current command-interface state as D-08."""
		self.status_publisher.publish(state="ready", run_list=self.registry.list_runs())
		print("D-01: " + ("loaded" if self.d01_json is not None else "not loaded"))
		print("run controls: " + (str(self.controls.as_dict()) if self.controls else "not set"))

	@staticmethod
	def print_help() -> None:
		"""Print the commands supported by the terminal interface."""
		print("Backend Controls")
		print("  load <file>                            load D-01 JSON")
		print("  settings <start> <end> <step> <rate>   set run controls")
		print("  status                                 publish D-08 status")
		print("  run                                    validate and start a run")
		print("  quit                                   exit")
