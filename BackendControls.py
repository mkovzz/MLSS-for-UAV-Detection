"""Composition root for the Backend Controls subsystem."""

from __future__ import annotations

from pathlib import Path

from CommandInterface import CommandInterface
from ConfigValidator import ConfigValidator
from RunController import RunController
from RunRegistry import RunRegistry
from StatusPublisher import StatusPublisher


class BackendControls:
    """Owns and coordinates one instance of each Backend Controls component."""

    def __init__(self, root: Path | None = None) -> None:
        """Create the components and inject them into the command interface."""
        self.root = root or Path(__file__).resolve().parent
        self.config_validator = ConfigValidator()
        self.run_registry = RunRegistry(self.root)
        self.run_controller = RunController()
        self.status_publisher = StatusPublisher(self.root / "D-08.json")
        self.command_interface = CommandInterface(
            root=self.root,
            config_validator=self.config_validator,
            run_registry=self.run_registry,
            run_controller=self.run_controller,
            status_publisher=self.status_publisher,
        )

    def run(self) -> int:
        """Run the command interface and guarantee component cleanup on exit."""
        try:
            self.command_interface.command_loop()
        except (EOFError, KeyboardInterrupt):
            print()
        finally:
            self.shutdown()
        return 0

    def shutdown(self) -> None:
        """Stop active component work before the subsystem process exits."""
        self.run_controller.stop_run()


def main() -> int:
    """Start one Backend Controls subsystem instance."""
    return BackendControls().run()


if __name__ == "__main__":
    raise SystemExit(main())
