#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
===============================================================================
File        : container.py
Project     : RideCast
Description : Container-side ESP-IDF firmware build and management utility.
Copyright   : Copyright (c) 2026 Navyantra Tech Private Limited
===============================================================================

Overview
--------
This script executes ESP-IDF firmware management operations from within
the RideCast Docker development environment.

It is invoked by the host-side docker/build.py script and provides a
consistent interface to the ESP-IDF command-line tool (idf.py).

The script supports the following operations:

    1. Build firmware.
    2. Clean firmware build artifacts.
    3. Perform a complete build cleanup.
    4. Open the ESP-IDF menuconfig interface.
    5. Flash firmware to a connected target.
    6. Monitor target UART output.
    7. Display firmware memory usage information.

Architecture
------------
Host System:
    docker/build.py
        |
        | Docker Compose
        v
Docker Container:
    docker/container.py
        |
        | ESP-IDF command-line interface
        v
    idf.py
        |
        v
    RideCast firmware project

Environment Configuration
-------------------------
Project root:
    /workspace/ridecast

ESP-IDF SDK:
    /workspace/ridecast/sdk/esp

Build output:
    /workspace/ridecast/build-docker

The script configures:

    IDF_PATH:
        Points to the ESP-IDF SDK included in the project.

    IDF_CCACHE_ENABLE:
        Set to 0 to disable ESP-IDF compiler caching.

The Docker image entry point is responsible for configuring compiler,
Python, and development-tool paths.

Requirements
------------
    - Python 3.10 or later.
    - ESP-IDF 5.5 SDK available at sdk/esp.
    - ESP-IDF development environment initialized by the Docker image.
    - RideCast firmware project mounted at /workspace/ridecast.

Usage Examples
--------------
Build firmware:
    python3 docker/container.py build

Clean firmware:
    python3 docker/container.py clean

Full clean:
    python3 docker/container.py fullclean

Open menuconfig:
    python3 docker/container.py menuconfig

Display firmware size:
    python3 docker/container.py size

Flash firmware:
    python3 docker/container.py flash --port /dev/ttyUSB0

Monitor UART output:
    python3 docker/container.py monitor --port /dev/ttyUSB0

Notes
-----
    - This script is intended to run inside the Docker environment.
    - Host operating-system handling is performed by docker/build.py.
    - All ESP-IDF operations execute from the RideCast project root.
    - Firmware build artifacts are stored in build-docker.
    - Command execution status is propagated to the calling process.

Change Log (Newest First)
-------------------------
2026-10-26  Documentation, formatting, and code organization updates.
             Initial container-side ESP-IDF runner.

===============================================================================
"""

from __future__ import annotations

# -----------------------------------------------------------------------------
# Standard Library Imports
# -----------------------------------------------------------------------------

import argparse
import os
from pathlib import Path
import subprocess
import sys

# -----------------------------------------------------------------------------
# Project Directory Configuration
# -----------------------------------------------------------------------------

# Root directory of the RideCast firmware project inside Docker.
ROOT = Path("/workspace/ridecast")

# Directory containing the project-managed ESP-IDF SDK.
SDK = ROOT / "sdk" / "esp"

# ESP-IDF CMake project definition used to validate SDK initialization.
PROJECT_FILE = SDK / "tools" / "cmake" / "project.cmake"

# ESP-IDF version information file.
VERSION_FILE = SDK / "version.txt"

# Directory used for firmware build artifacts.
BUILD_DIR = "build-docker"


# -----------------------------------------------------------------------------
# Supported Firmware Operations
# -----------------------------------------------------------------------------

# List of firmware management actions accepted by this script.
ACTIONS = (
    "build",
    "clean",
    "fullclean",
    "menuconfig",
    "flash",
    "monitor",
    "size",
)


# -----------------------------------------------------------------------------
# Application Entry Function
# -----------------------------------------------------------------------------


def main() -> int:
    """
    Validate the ESP-IDF environment and execute a firmware operation.

    Returns:
        int:
            Exit status returned by the ESP-IDF command.

    Raises:
        OSError:
            Raised when the ESP-IDF command cannot be started.

    Notes:
        The requested action is forwarded directly to idf.py.

        The ESP-IDF SDK directory and version are validated before
        command execution.

        Flashing and monitoring require a serial device path.
    """

    # Initialize command-line argument parsing.
    parser = argparse.ArgumentParser(description="RideCast container ESP-IDF runner")

    # Select the firmware operation to execute.
    parser.add_argument(
        "action",
        choices=ACTIONS,
    )

    # Optional serial device for flashing or monitoring.
    parser.add_argument(
        "--port",
        default=None,
    )

    # Serial communication baud rate.
    parser.add_argument(
        "--baud",
        type=int,
        default=460800,
    )

    # Parse command-line arguments.
    args = parser.parse_args()

    # -------------------------------------------------------------------------
    # ESP-IDF SDK Validation
    # -------------------------------------------------------------------------

    # Verify that the ESP-IDF SDK contains the expected CMake project file.
    if not PROJECT_FILE.is_file():
        parser.error(
            "sdk/esp must be initialized: " "git submodule update --init --recursive"
        )

    # Validate the SDK version when version information is available.
    if VERSION_FILE.is_file():

        # Read the configured ESP-IDF release version.
        version = VERSION_FILE.read_text(encoding="utf-8").strip()

        # Reject SDK versions outside the required ESP-IDF 5.5 series.
        if not version.startswith("5.5"):
            parser.error(
                f"sdk/esp version {version!r} is not ESP-IDF 5.5; "
                "rebuild with a matching image"
            )

    # -------------------------------------------------------------------------
    # ESP-IDF Environment Configuration
    # -------------------------------------------------------------------------

    # Preserve environment variables provided by the Docker image.
    environment = os.environ.copy()

    # Select the project-managed ESP-IDF SDK.
    environment["IDF_PATH"] = str(SDK)

    # Disable compiler caching for the firmware build.
    environment["IDF_CCACHE_ENABLE"] = "0"

    # The Docker image entry point already configures the compiler,
    # Python environment, and required ESP-IDF development tools.

    # -------------------------------------------------------------------------
    # ESP-IDF Command Construction
    # -------------------------------------------------------------------------

    # Construct the base ESP-IDF command with a dedicated build directory.
    command = [
        "idf.py",
        "-B",
        BUILD_DIR,
    ]

    # Flash and monitor operations require a target serial device.
    if args.action in ("flash", "monitor"):

        # Reject flash/monitor requests without a serial port.
        if not args.port:
            parser.error(f"{args.action} requires --port")

        # Forward the selected port and baud rate to ESP-IDF.
        command.extend(
            [
                "-p",
                args.port,
                "-b",
                str(args.baud),
            ]
        )

    # Append the requested firmware management action.
    command.append(args.action)

    # -------------------------------------------------------------------------
    # ESP-IDF Command Execution
    # -------------------------------------------------------------------------

    # Display the command to simplify build and execution debugging.
    print(
        "Running:",
        " ".join(command),
        flush=True,
    )

    # Execute ESP-IDF from the firmware project root.
    # Return its exit code to Docker and the host-side build utility.
    result = subprocess.run(
        command,
        cwd=ROOT,
        env=environment,
        check=False,
    )

    return result.returncode


# -----------------------------------------------------------------------------
# Application Entry Point
# -----------------------------------------------------------------------------

if __name__ == "__main__":
    sys.exit(main())


# -----------------------------------------------------------------------------
# End of File
# -----------------------------------------------------------------------------
