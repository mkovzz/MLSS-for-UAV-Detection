"""Run Registry component and persisted run-list management."""

from __future__ import annotations

from datetime import datetime, timezone
import json
from pathlib import Path
import uuid
from typing import Any


class RunRegistry:
	"""Create run directories and maintain the persisted D-08 run list."""

	def __init__(self, root: Path | None = None) -> None:
		"""Initialize registry storage below the selected workspace."""
		workspace = root or Path(__file__).resolve().parent
		self.root = workspace / "runs"
		self.run_list_path = self.root / "run-list.json"
		self.root.mkdir(parents=True, exist_ok=True)

	def create_run(self, controls: dict[str, Any]) -> tuple[str, Path]:
		"""Create a unique run entry and return its identifier and directory."""
		run_id = f"run-{datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')}-{uuid.uuid4().hex[:8]}"
		run_path = self.root / run_id
		run_path.mkdir()
		entry = {
			"runId": run_id,
			"path": str(run_path),
			"createdAt": datetime.now(timezone.utc).isoformat(),
			"state": "created",
			"controls": controls,
		}
		entries = self._read_entries()
		entries.append(entry)
		self._write_entries(entries)
		return run_id, run_path

	def update_run(self, run_id: str, **updates: Any) -> None:
		"""Update the stored metadata for one registered run."""
		entries = self._read_entries()
		for entry in entries:
			if entry.get("runId") == run_id:
				entry.update(updates)
				break
		self._write_entries(entries)

	def list_runs(self) -> list[dict[str, Any]]:
		"""Return the current persisted run list."""
		return self._read_entries()

	def _read_entries(self) -> list[dict[str, Any]]:
		"""Read the run list, treating missing or damaged state as empty."""
		if not self.run_list_path.exists():
			return []
		try:
			value = json.loads(self.run_list_path.read_text(encoding="utf-8"))
		except (OSError, json.JSONDecodeError):
			return []
		return value if isinstance(value, list) else []

	def _write_entries(self, entries: list[dict[str, Any]]) -> None:
		"""Replace the run list atomically to avoid partial JSON files."""
		temporary = self.run_list_path.with_suffix(".tmp")
		temporary.write_text(json.dumps(entries, indent=2) + "\n", encoding="utf-8")
		temporary.replace(self.run_list_path)
