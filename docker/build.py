#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
===============================================================================
File        : build.py
Project     : RideCast
Description : Cross-platform Docker build and firmware management utility.
Copyright   : Copyright (c) 2026 Navyantra Tech Private Limited
===============================================================================

Overview
--------
This script provides a unified command-line interface for managing RideCast
firmware development using an ESP-IDF 5.5 Docker environment.

The script acts as a host-side wrapper around Docker Compose and the
container-side firmware management script (docker/container.py).

It supports the following operations:

    1. Build the Docker toolchain image.
    2. Build the RideCast firmware.
    3. Clean existing firmware build artifacts.
    4. Perform a complete firmware build cleanup.
    5. Open the ESP-IDF menuconfig interface.
    6. Display firmware memory usage information.
    7. Flash firmware to a connected ESP32 target.
    8. Monitor UART output from the target device.

Platform Support
----------------
Windows:
    - Firmware compilation executes inside Docker.
    - Firmware flashing executes on the Windows host using esptool.
    - UART monitoring executes on the Windows host using pyserial.

Linux:
    - Firmware compilation executes inside Docker.
    - Flashing and monitoring execute inside Docker.
    - Serial devices are passed into the container.
    - Host user and group IDs are forwarded to prevent root-owned files.

Requirements
------------
    - Python 3.10 or later.
    - Docker with Docker Compose support.
    - ESP-IDF Docker configuration in docker/compose.yaml.
    - docker/container.py for container-side operations.

Additional Windows Requirements
-------------------------------
    - esptool for firmware flashing.
    - pyserial for UART monitoring.

Usage Examples
--------------
Build firmware:
    python docker/build.py --build

Full clean and rebuild:
    python docker/build.py --fullclean --build

Build the Docker image:
    python docker/build.py --image-build

Flash firmware on Windows:
    python docker/build.py --flash --port COM7

Flash firmware on Linux:
    python docker/build.py --flash --port /dev/ttyUSB0

Monitor UART output:
    python docker/build.py --monitor --port COM7

Display firmware size:
    python docker/build.py --size

Open menuconfig:
    python docker/build.py --menuconfig

Notes
-----
    - Firmware artifacts are stored in the build-docker directory.
    - Windows flashing uses the ESP-IDF-generated flasher_args.json file.
    - Flash image paths are validated before they are passed to esptool.
    - This script does not directly implement firmware compilation.
    - Docker Compose and docker/container.py perform container operations.

Change Log (Newest First)
-------------------------
2026-10-26  Code formatting, documentation, and structure improvements.
2026-10-25  Initial Docker firmware management utility.

===============================================================================
"""

from __future__ import annotations

# -----------------------------------------------------------------------------
# Standard Library Imports
# -----------------------------------------------------------------------------

import argparse
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

# -----------------------------------------------------------------------------
# Project Directory Configuration
# -----------------------------------------------------------------------------

# Directory containing this Docker management script.
DOCKER_DIR = Path(__file__).resolve().parent

# Root directory of the RideCast firmware project.
ROOT = DOCKER_DIR.parent

# Docker Compose configuration file.
COMPOSE = DOCKER_DIR / "compose.yaml"

# Directory containing Docker-generated firmware build artifacts.
OUTPUT = ROOT / "build-docker"


# -----------------------------------------------------------------------------
# Internal Function Declarations
# -----------------------------------------------------------------------------
# Python functions are defined before the main entry point.
# -----------------------------------------------------------------------------


def execute(command: list[str], *, cwd: Path = ROOT) -> None:
    """
    Execute a host operating-system command.

    Args:
        command:
            Command and its arguments as a list of strings.

        cwd:
            Working directory used during command execution.

    Raises:
        subprocess.CalledProcessError:
            Raised when the command returns a nonzero exit status.

        OSError:
            Raised when the command cannot be executed.

    Notes:
        Commands are executed without a shell to avoid shell
        interpretation of command arguments.
    """

    # Display the command to assist with build and execution debugging.
    print("+", " ".join(command), flush=True)

    # Execute the command and report failure through an exception.
    subprocess.run(command, cwd=cwd, check=True)


# -----------------------------------------------------------------------------


def in_docker(
    action: str,
    port: str | None = None,
    baud: int = 460800,
) -> None:
    """
    Execute a firmware management operation inside Docker.

    Args:
        action:
            Requested firmware operation, such as build, clean, flash,
            monitor, menuconfig, or size.

        port:
            Optional host serial device used for flashing or monitoring.

        baud:
            Serial communication baud rate.

    Raises:
        ValueError:
            Raised when a Linux serial device path is invalid.

        OSError:
            Raised when serial device information cannot be retrieved.

        subprocess.CalledProcessError:
            Raised when Docker Compose returns an error.

    Notes:
        Linux user and group IDs are forwarded into the container to
        prevent build artifacts from being owned by root.

        Serial devices are passed to Docker only for Linux flashing
        and monitoring operations.
    """

    # Construct the base Docker Compose command.
    command = [
        "docker",
        "compose",
        "-f",
        str(COMPOSE),
        "run",
        "--rm",
        "--no-deps",
    ]

    # Apply Linux-specific container execution settings.
    if platform.system() == "Linux":

        # Run container processes using the current host user and group.
        command.extend(
            [
                "--user",
                f"{os.getuid()}:{os.getgid()}",
                "-e",
                "HOME=/tmp",
            ]
        )

        # Serial device passthrough is required only for flash/monitor.
        if action in ("flash", "monitor"):

            # Reject missing or non-device serial port paths.
            if not port or not port.startswith("/dev/"):
                raise ValueError(
                    "Linux Docker flash/monitor requires "
                    "/dev/ttyUSB* or /dev/ttyACM* port"
                )

            # Retrieve the device group to permit serial access.
            device_group = os.stat(port).st_gid

            # Map the host serial device into the container.
            command.extend(
                [
                    "--device",
                    f"{port}:{port}",
                    "--group-add",
                    str(device_group),
                ]
            )

    # Append the service and container-side Python entry point.
    command.extend(
        [
            "ridecast",
            "python3",
            "docker/container.py",
            action,
        ]
    )

    # Forward the serial port when provided.
    if port is not None:
        command.extend(["--port", port])

    # Forward baud rate only for serial communication operations.
    if action in ("flash", "monitor"):
        command.extend(["--baud", str(baud)])

    # Execute the completed Docker Compose command.
    execute(command)


# -----------------------------------------------------------------------------


def host_flash(port: str, baud: int) -> None:
    """
    Flash RideCast firmware using the host Python environment.

    Args:
        port:
            Serial port connected to the target microcontroller.

        baud:
            Flashing baud rate.

    Raises:
        FileNotFoundError:
            Raised when the flash manifest or an image file is missing.

        ValueError:
            Raised when the flash manifest contains invalid configuration.

        subprocess.CalledProcessError:
            Raised when esptool fails.

    Notes:
        The flash configuration is loaded from the ESP-IDF-generated
        flasher_args.json file.

        Flash offsets and image filenames are taken from the manifest
        rather than being hardcoded in this script.

        Firmware image paths are restricted to the build output
        directory before they are supplied to esptool.
    """

    # Locate the ESP-IDF flashing configuration.
    manifest = OUTPUT / "flasher_args.json"

    # A successful firmware build is required before flashing.
    if not manifest.is_file():
        raise FileNotFoundError(
            "Missing build-docker/flasher_args.json; " "build firmware first"
        )

    # Read and parse the flashing configuration.
    configuration = json.loads(manifest.read_text(encoding="utf-8"))

    # Read optional esptool configuration arguments.
    settings = configuration.get("extra_esptool_args", {})

    # Extract target and reset options with default values.
    chip = settings.get("chip", "esp32")
    before = settings.get("before", "default_reset")
    after = settings.get("after", "hard_reset")

    # Read flash write options and firmware image mappings.
    write_args = configuration.get("write_flash_args", [])
    flash_files = configuration.get("flash_files", {})

    # Validate the required flash configuration entries.
    if (
        not isinstance(write_args, list)
        or not isinstance(flash_files, dict)
        or not flash_files
    ):
        raise ValueError("Invalid flasher_args.json flash configuration")

    # Construct the host-side esptool command.
    command = [
        sys.executable,
        "-m",
        "esptool",
        "--chip",
        str(chip),
        "--port",
        port,
        "--baud",
        str(baud),
        "--before",
        str(before),
        "--after",
        str(after),
    ]

    # Disable the flashing stub when specified by the manifest.
    if settings.get("stub") is False:
        command.append("--no-stub")

    # Add the flashing operation and configured write arguments.
    command.extend(
        [
            "write_flash",
            *[str(item) for item in write_args],
        ]
    )

    # Resolve the build output directory once for path validation.
    output_dir = OUTPUT.resolve()

    # Add every firmware image and its configured flash address.
    for address, filename in flash_files.items():

        # Resolve the firmware image relative to the build directory.
        image_path = (OUTPUT / filename).resolve()

        # Reject files outside the output directory or missing images.
        if not image_path.is_relative_to(output_dir) or not image_path.is_file():
            raise FileNotFoundError(f"Invalid or missing flash image: {filename}")

        # Append the flash offset followed by the image file path.
        command.extend(
            [
                str(address),
                str(image_path),
            ]
        )

    # Flash the configured images using the host Python environment.
    execute(command)


# -----------------------------------------------------------------------------
# Command-Line Interface
# -----------------------------------------------------------------------------


def main() -> int:
    """
    Parse command-line arguments and execute requested operations.

    Returns:
        int:
            0 when all requested operations complete successfully.
            1 when an operation fails.

    Notes:
        Operations are executed sequentially in a predefined order.

        The --action argument selects a single operation, whereas
        individual flags may be combined to execute multiple operations.
    """

    # Create the command-line argument parser.
    parser = argparse.ArgumentParser(
        description="RideCast ESP-IDF 5.5 Docker build/flash"
    )

    # Define an optional single-action argument group.
    group = parser.add_mutually_exclusive_group()

    group.add_argument(
        "--action",
        choices=(
            "build",
            "clean",
            "fullclean",
            "menuconfig",
            "flash",
            "monitor",
            "size",
        ),
    )

    # Firmware build operations.
    parser.add_argument(
        "-b",
        "--build",
        action="store_true",
    )

    parser.add_argument(
        "-c",
        "--clean",
        action="store_true",
    )

    parser.add_argument(
        "-fc",
        "--fullclean",
        action="store_true",
    )

    # Device communication operations.
    parser.add_argument(
        "-f",
        "--flash",
        action="store_true",
    )

    parser.add_argument(
        "-m",
        "--monitor",
        action="store_true",
    )

    # Firmware configuration and analysis operations.
    parser.add_argument(
        "--menuconfig",
        action="store_true",
    )

    parser.add_argument(
        "--size",
        action="store_true",
    )

    # Serial port selection for flash and monitor.
    parser.add_argument(
        "-p",
        "--port",
        help="COM7 (Windows) or /dev/ttyUSB0 (Linux)",
    )

    # Serial communication baud rate.
    parser.add_argument(
        "--baud",
        type=int,
        default=460800,
    )

    # Explicit Docker toolchain image build.
    parser.add_argument(
        "--image-build",
        action="store_true",
        help="Build/rebuild toolchain image",
    )

    # Parse user-supplied command-line arguments.
    args = parser.parse_args()

    # Store the operations selected by the user.
    tasks = []

    # Execute a single selected action when specified.
    if args.action:
        tasks.append(args.action)

    else:

        # Collect enabled operations in the required execution order.
        operations = (
            (args.fullclean, "fullclean"),
            (args.clean, "clean"),
            (args.menuconfig, "menuconfig"),
            (args.build, "build"),
            (args.size, "size"),
            (args.flash, "flash"),
            (args.monitor, "monitor"),
        )

        for enabled, name in operations:
            if enabled:
                tasks.append(name)

    # Prevent conflicting cleanup operations.
    if "fullclean" in tasks and "clean" in tasks:
        parser.error("Use either --clean or --fullclean")

    # Require at least one firmware or Docker image operation.
    if not tasks and not args.image_build:
        parser.error("Select --build, --flash, --fullclean, " "--menuconfig, etc.")

    # Ensure a serial port is provided for flash and monitor.
    if any(task in ("flash", "monitor") for task in tasks) and not args.port:
        parser.error("--port is required for --flash or --monitor")

    try:

        # Build or rebuild the Docker firmware toolchain image.
        if args.image_build:
            execute(
                [
                    "docker",
                    "compose",
                    "-f",
                    str(COMPOSE),
                    "build",
                    "ridecast",
                ]
            )

        # Execute each selected firmware management operation.
        for task in tasks:

            # Windows flashing uses the host serial interface.
            if task == "flash" and platform.system() == "Windows":
                host_flash(args.port, args.baud)

            # Windows monitoring uses pyserial on the host.
            elif task == "monitor" and platform.system() == "Windows":

                execute(
                    [
                        sys.executable,
                        "-m",
                        "serial.tools.miniterm",
                        args.port,
                        "115200",
                    ]
                )

            # All other operations execute inside Docker.
            else:
                in_docker(
                    task,
                    args.port if task in ("flash", "monitor") else None,
                    args.baud,
                )

    # Convert operational failures into a nonzero process exit status.
    except (
        ValueError,
        FileNotFoundError,
        subprocess.CalledProcessError,
        OSError,
    ) as exc:

        print(
            f"[RideCast Docker] {exc}",
            file=sys.stderr,
        )

        return 1

    # All requested operations completed successfully.
    return 0


# -----------------------------------------------------------------------------
# Application Entry Point
# -----------------------------------------------------------------------------

if __name__ == "__main__":
    sys.exit(main())


# -----------------------------------------------------------------------------
# End of File
# -----------------------------------------------------------------------------
