#!/usr/bin/env python3

# ====================================================
# Ridecast Build Utility (Cross-platform ESP-IDF helper)
# ----------------------------------------------------
# Design notes (MISRA-inspired defensive style):
# 1) Validate inputs and paths before executing commands.
# 2) Keep OS-specific logic centralized.
# 3) Avoid hidden side effects (helpers raise errors; main exits once).
# 4) Keep each function single-purpose and documented.
# ====================================================

import argparse
import logging
import os
import platform
import shlex
import subprocess
import sys
from typing import List, Optional

# ----------------------------------------------------
#   COLOR LOGGER (Windows Compatible)
# ----------------------------------------------------


class ColorFormatter(logging.Formatter):
    """Simple ANSI color formatter for log levels."""

    COLORS = {
        "DEBUG": "\033[94m",
        "INFO": "\033[92m",
        "WARNING": "\033[93m",
        "ERROR": "\033[91m",
        "CRITICAL": "\033[95m",
    }
    RESET = "\033[0m"

    def format(self, record: logging.LogRecord) -> str:
        color = self.COLORS.get(record.levelname, "")
        message = super().format(record)
        return f"{color}{message}{self.RESET}"


def setup_logger(debug_enabled: bool) -> logging.Logger:
    """
    API: setup_logger(debug_enabled)
    Purpose: Configure and return logger for this tool.
    """
    level = logging.DEBUG if debug_enabled else logging.INFO
    logger = logging.getLogger("ridecast-build")
    logger.setLevel(level)

    # Prevent duplicate handlers when script is reloaded/run repeatedly.
    if not logger.handlers:
        handler = logging.StreamHandler()
        handler.setFormatter(ColorFormatter("%(levelname)s: %(message)s"))
        logger.addHandler(handler)

    logger.propagate = False
    return logger


# ----------------------------------------------------
#   Helper: RUN COMMAND
# ----------------------------------------------------
def run_cmd(cmd: str, logger: logging.Logger) -> None:
    """
    API: run_cmd(cmd, logger)
    Purpose: Execute shell command and fail fast on non-zero exit code.
    """
    logger.debug("Execute: %s", cmd)
    result = subprocess.run(cmd, shell=True, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"Command failed with code {result.returncode}: {cmd}")


# ----------------------------------------------------
#   Detect OS
# ----------------------------------------------------
def is_windows() -> bool:
    """
    API: is_windows()
    Purpose: Return True when running on Windows.
    """
    return platform.system().lower().startswith("win")


def is_unix() -> bool:
    """
    API: is_unix()
    Purpose: Return True for Linux/macOS.
    """
    return platform.system().lower() in ("linux", "darwin")

def idf_set_target(target: str, export_script: str, logger: logging.Logger) -> None:
    """
    API: idf_set_target(target, export_script, logger)
    Purpose: Run 'idf.py set-target <target>'.
    """
    if (target is None) or (target.strip() == ""):
        raise ValueError("Target must not be empty")

    logger.info("Set target... (%s)", target)
    cmd = build_idf_command(export_script, ["set-target", target])
    run_cmd(cmd, logger)
    
def print_help_banner() -> None:
    """
    API: print_help_banner()
    Purpose: Print a detailed, human-readable usage guide for the build utility.
             Covers all flags, single-use and combined usage examples.
    """
    banner = """
\033[96m╔══════════════════════════════════════════════════════════════╗
║           RideCast Build Utility - Help & Usage Guide        ║
╚══════════════════════════════════════════════════════════════╝\033[0m

\033[93mDESCRIPTION:\033[0m
  Cross-platform ESP-IDF build helper for the RideCast project.
  Wraps idf.py commands with automatic environment setup.

\033[93mUSAGE:\033[0m
  python build.py [OPTIONS]

\033[93mOPTIONS:\033[0m
  \033[92m-t, --target\033[0m      Set chip target (e.g. esp32s3)
  \033[92m-b, --build\033[0m       Build the project (idf.py build)
  \033[92m-c, --clean\033[0m       Full clean build directory (idf.py fullclean)
  \033[92m-f, --flash\033[0m       Flash firmware to connected device (idf.py flash)
  \033[92m-m, --monitor\033[0m     Open serial monitor for device output (idf.py monitor)
  \033[92m-l, --logdebug\033[0m    Enable verbose/debug logging for this script
  \033[92m--all\033[0m             Run all actions: clean → build → flash → monitor
  \033[92m--help-usage\033[0m      Show this detailed help and usage guide
  \033[92m-h, --help\033[0m        Show short argument reference

\033[93mSINGLE ACTION EXAMPLES:\033[0m
  python build.py -t esp32s3       # Set target only
  python build.py -b              # Build only
  python build.py -c              # Clean only
  python build.py -f              # Flash only
  python build.py -m              # Monitor only
  python build.py -l -b           # Build with debug logging

\033[93mMULTIPLE ACTION EXAMPLES:\033[0m
  python build.py -c -b           # Clean then build
  python build.py -b -f           # Build then flash
  python build.py -b -f -m        # Build, flash, then monitor
  python build.py -c -b -f -m     # Full pipeline manually
  python build.py --all           # Full pipeline (shorthand)
  python build.py --all -l        # Full pipeline with debug logs

\033[93mACTION EXECUTION ORDER:\033[0m
  Regardless of flag order, actions always run in this sequence:
  \033[92m1. clean  →  2. build  →  3. flash  →  4. monitor\033[0m

\033[93mNOTES:\033[0m
  • ESP-IDF SDK must exist at: sdk/esp/
  • On Windows: sdk/esp/export.bat must be present
  • On Linux/macOS: sdk/esp/export.sh must be present
  • Run 'git submodule update --init --recursive' if SDK is missing

\033[96m══════════════════════════════════════════════════════════════\033[0m
"""
    print(banner)

# ----------------------------------------------------
def validate_export_script(idf_export_base: str) -> str:
    """
    API: validate_export_script(idf_export_base)
    Purpose: Resolve and validate export script path by OS.
    Returns: Full export script path (export.bat or export.sh).
    """
    if is_windows():
        path = idf_export_base + ".bat"
    elif is_unix():
        path = idf_export_base + ".sh"
    else:
        raise RuntimeError("Unsupported operating system")

    if not os.path.isfile(path):
        raise FileNotFoundError(f"ESP-IDF export script not found: {path}")

    return path


# ----------------------------------------------------
#   IDF BUILD / CLEAN
# ----------------------------------------------------
def build_idf_command(export_script: str, idf_args: List[str]) -> str:
    """
    API: build_idf_command(export_script, idf_args)
    Purpose: Build a single command line that:
             1) loads ESP-IDF environment, then
             2) runs idf.py with provided arguments.
    Why: environment setup must happen in the same shell process.
    """
    idf_part = "idf.py " + " ".join(shlex.quote(x) for x in idf_args)

    if is_windows():
        # cmd.exe quoting style
        script = export_script.replace("/", "\\")
        return f'cmd /d /s /c "{script} && {idf_part}"'

    # bash -lc keeps env in same shell session for the command chain
    script_q = shlex.quote(export_script)
    return f"bash -lc 'source {script_q} && {idf_part}'"

# ----------------------------------------------------

def idf_action(action: str, export_script: str, logger: logging.Logger, port: Optional[str] = None) -> None:
    """
    API: idf_action(action, export_script, logger, port)
    Purpose: Run one ESP-IDF action.
    Supported actions: clean, build, flash, monitor
    """
    mapping = {
        "clean": ["fullclean"],
        "build": ["build"],
        "flash": ["flash"],
        "monitor": ["monitor"],
    }

    if action not in mapping:
        raise ValueError(f"Unsupported action: {action}")

    idf_args = list(mapping[action])

    # Port is relevant for flash/monitor
    if (action in ("flash", "monitor")) and port:
        idf_args.extend(["-p", port])

    if (action in ("flash", "monitor")) and port:
        logger.info("%s... (port=%s)", action.capitalize(), port)
    else:
        logger.info("%s...", action.capitalize())

    cmd = build_idf_command(export_script, idf_args)
    run_cmd(cmd, logger)

# ----------------------------------------------------
#   MAIN
# ----------------------------------------------------
def main() -> int:
    """
    API: main()
    Purpose: Parse CLI, validate environment, execute selected actions.
    Returns: process exit code.
    """
    parser = argparse.ArgumentParser(
        description="RideCast Build Utility (Cross Platform)",
        add_help=True
    )
    parser.add_argument("-t", "--target",     type=str,            help="Set IDF target (e.g. esp32s3)")
    parser.add_argument("-b", "--build",      action="store_true", help="Build the project")
    parser.add_argument("-c", "--clean",      action="store_true", help="Full clean the project")
    parser.add_argument("-f", "--flash",      action="store_true", help="Flash firmware to device")
    parser.add_argument("-m", "--monitor",    action="store_true", help="Open serial monitor")
    parser.add_argument("-p", "--port",       type=str,            help="Serial port (e.g. COM3, /dev/ttyUSB0)")
    parser.add_argument("-l", "--logdebug",   action="store_true", help="Enable verbose debug logging")
    parser.add_argument("--all",              action="store_true", help="Run: clean → build → flash → monitor")
    parser.add_argument("--help-usage",       action="store_true", help="Show detailed usage guide with examples")
    args = parser.parse_args()

    # Show detailed help banner and exit cleanly
    if args.help_usage:
        print_help_banner()
        return 0

    if args.all:
        args.clean   = True
        args.build   = True
        args.flash   = True
        args.monitor = True

    logger = setup_logger(args.logdebug)

    try:
        script_root = os.path.dirname(os.path.abspath(__file__))
        sdk_base = os.path.join(script_root, "sdk", "esp")
        idf_export_base = os.path.join(sdk_base, "export")

        logger.debug("SDK path: %s", sdk_base)
        logger.debug("Export script base: %s", idf_export_base)

        if not (args.clean or args.build or args.flash or args.monitor):
            logger.warning("No action selected. Use -b, -c, -f, -m or --all.")
            return 0

        if (args.flash or args.monitor) and (not args.port):
            logger.warning("No --port provided for flash/monitor. Using ESP-IDF default port selection.")

        export_script = validate_export_script(idf_export_base)

        # Fixed, deterministic order for reproducible operation
        if args.clean:
            idf_action("clean", export_script, logger, args.port)
        if args.build:
            idf_action("build", export_script, logger, args.port)
        if args.flash:
            idf_action("flash", export_script, logger, args.port)
        if args.monitor:
            idf_action("monitor", export_script, logger, args.port)

        logger.info("Done.")
        return 0

    except Exception as exc:
        logger.error("%s", exc)
        return 1


if __name__ == "__main__":
    sys.exit(main())
