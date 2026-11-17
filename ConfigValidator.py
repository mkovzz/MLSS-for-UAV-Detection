"""Validation component for the D-01 scenario configuration asset."""

from __future__ import annotations

from dataclasses import dataclass
import json
from typing import Any


@dataclass(frozen=True)
class ValidationError:
	"""Describe one validation failure and its location in D-01."""
	path: str
	message: str

	def as_dict(self) -> dict[str, str]:
		"""Convert the error to a D-08-friendly dictionary."""
		return {"path": self.path, "message": self.message}


@dataclass(frozen=True)
class ValidatedConfig:
	"""Hold the original D-01 JSON and its parsed object after validation."""
	raw_json: str
	document: dict[str, Any]


@dataclass(frozen=True)
class ValidationResult:
	"""Contain either a validated D-01 asset or its rejection errors."""
	config: ValidatedConfig | None
	errors: tuple[ValidationError, ...]

	@property
	def is_valid(self) -> bool:
		"""Return whether validation produced no errors."""
		return not self.errors


class ConfigValidator:
	"""Validate D-01 syntax and generic invariants without rewriting it."""

	def validate(self, raw_json: str) -> ValidationResult:
		"""Parse and validate raw D-01 text while preserving the original string."""
		errors: list[ValidationError] = []
		if not raw_json.strip():
			return ValidationResult(None, (ValidationError("$", "configuration is empty"),))

		try:
			document = json.loads(raw_json)
		except json.JSONDecodeError as error:
			errors.append(ValidationError(f"line {error.lineno}, column {error.colno}", error.msg))
			return ValidationResult(None, tuple(errors))

		if not isinstance(document, dict):
			errors.append(ValidationError("$", "D-01 must contain a JSON object"))
			return ValidationResult(None, tuple(errors))

		self._validate_array(document, "groundStations", errors)
		self._validate_aircraft(document, errors)
		self._validate_array(document, "satellites", errors)
		self._validate_unique_ids(document, errors)
		self._validate_run_settings(document, errors)

		# The raw text is retained so the physics engine receives D-01 unchanged.
		config = None if errors else ValidatedConfig(raw_json, document)
		return ValidationResult(config, tuple(errors))

	@staticmethod
	def _validate_array(
		document: dict[str, Any], key: str, errors: list[ValidationError]
	) -> None:
		"""Check that an optional scenario section is represented as an array."""
		if key in document and not isinstance(document[key], list):
			errors.append(ValidationError(f"$.{key}", "must be an array when provided"))

	@staticmethod
	def _validate_aircraft(document: dict[str, Any], errors: list[ValidationError]) -> None:
		"""Check that aircraft generation settings use the D-01 object shape."""
		aircraft = document.get("aircraft")
		if aircraft is not None and not isinstance(aircraft, dict):
			errors.append(ValidationError("$.aircraft", "must be an object when provided"))

	@staticmethod
	def _validate_unique_ids(document: dict[str, Any], errors: list[ValidationError]) -> None:
		"""Check IDs only for sections represented as entity arrays."""
		for key in ("groundStations", "aircraft", "satellites"):
			values = document.get(key)
			if not isinstance(values, list):
				continue
			seen: set[str] = set()
			for index, item in enumerate(values):
				if not isinstance(item, dict):
					errors.append(ValidationError(f"$.{key}[{index}]", "must be an object"))
					continue
				identifier = item.get("id")
				if identifier is None:
					continue
				if not isinstance(identifier, str) or not identifier.strip():
					errors.append(ValidationError(f"$.{key}[{index}].id", "must be a non-empty string"))
				elif identifier in seen:
					errors.append(ValidationError(f"$.{key}[{index}].id", "must be unique"))
				else:
					seen.add(identifier)

	@staticmethod
	def _validate_run_settings(document: dict[str, Any], errors: list[ValidationError]) -> None:
		"""Check optional run timing values without requiring them in D-01."""
		settings = document.get("runSettings")
		if settings is None:
			return
		if not isinstance(settings, dict):
			errors.append(ValidationError("$.runSettings", "must be an object"))
			return
		numeric = ("startTimeS", "endTimeS", "stepSizeS")
		for name in numeric:
			if name in settings and (isinstance(settings[name], bool) or not isinstance(settings[name], (int, float))):
				errors.append(ValidationError(f"$.runSettings.{name}", "must be numeric"))
		if "startTimeS" in settings and "endTimeS" in settings:
			if settings["endTimeS"] <= settings["startTimeS"]:
				errors.append(ValidationError("$.runSettings", "endTimeS must be greater than startTimeS"))
		if "stepSizeS" in settings and settings["stepSizeS"] <= 0:
			errors.append(ValidationError("$.runSettings.stepSizeS", "must be greater than zero"))
