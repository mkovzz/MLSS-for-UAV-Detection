"""Status Publisher component for the D-08 run-status asset."""

from __future__ import annotations

from datetime import datetime, timezone
import json
from pathlib import Path
from typing import Any, Iterable


class StatusPublisher:
	"""Publish the current run lifecycle as the D-08 status asset."""

	def __init__(self, output_path: Path | None = None) -> None:
		"""Initialize in-memory status and an optional JSON output path."""
		self.output_path = output_path
		self._status: dict[str, Any] = {
			"asset": "D-08",
			"runState": "idle",
			"progress": 0.0,
			"etaSeconds": None,
			"errorText": None,
			"runList": [],
		}

	def publish(
		self,
		*,
		state: str,
		progress: float = 0.0,
		eta_seconds: float | None = None,
		error_text: str | None = None,
		run_list: Iterable[dict[str, Any]] = (),
		run_id: str | None = None,
	) -> dict[str, Any]:
		"""Update, persist, and report one D-08 status snapshot."""
		# Clamp progress so callers cannot publish an invalid D-08 percentage.
		self._status = {
			"asset": "D-08",
			"updatedAt": datetime.now(timezone.utc).isoformat(),
			"runId": run_id,
			"runState": state,
			"progress": max(0.0, min(1.0, progress)),
			"etaSeconds": eta_seconds,
			"errorText": error_text,
			"runList": list(run_list),
		}
		if self.output_path is not None:
			# Replace through a temporary file so readers never see partial JSON.
			self.output_path.parent.mkdir(parents=True, exist_ok=True)
			temporary = self.output_path.with_suffix(".tmp")
			temporary.write_text(json.dumps(self._status, indent=2) + "\n", encoding="utf-8")
			temporary.replace(self.output_path)
		print(
			f"[StatusPublisher] {state}"
			+ (f": {error_text}" if error_text else ""),
			flush=True,
		)
		return self._status.copy()

	@property
	def status(self) -> dict[str, Any]:
		"""Return a copy of the most recently published status."""
		return self._status.copy()
