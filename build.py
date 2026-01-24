#!/usr/bin/env python3
import argparse
import subprocess
import os
import sys
import logging
import platform

# ----------------------------------------------------
#   COLOR LOGGER (Windows Compatible)
# ----------------------------------------------------
class ColorFormatter(logging.Formatter):
    COLORS = {
        "DEBUG": "\033[94m",
        "INFO": "\033[92m",
        "WARNING": "\033[93m",
        "ERROR": "\033[91m",
        "CRITICAL": "\033[95m",
    }
    RESET = "\033[0m"

    def format(self, record):
        color = self.COLORS.get(record.levelname, "")
        message = super().format(record)
        return f"{color}{message}{self.RESET}"

def setup_logger(debug_enabled: bool):
    level = logging.DEBUG if debug_enabled else logging.INFO
    logger = logging.getLogger("ridecast-build")
    logger.setLevel(level)

    handler = logging.StreamHandler()
    handler.setFormatter(ColorFormatter("%(levelname)s: %(message)s"))

    logger.addHandler(handler)
    return logger

# ----------------------------------------------------
#   Helper: RUN COMMAND
# ----------------------------------------------------
def run_cmd(cmd, logger, shell=True):
    logger.debug(f"Execute: {cmd}")
    result = subprocess.call(cmd, shell=shell)
    if result != 0:
        logger.error("Command failed!")
        sys.exit(result)

# ----------------------------------------------------
#   Detect OS
# ----------------------------------------------------
def is_windows():
    return platform.system().lower().startswith("win")

def is_unix():
    return platform.system().lower() in ["linux", "darwin"]

# ----------------------------------------------------
#   Source ESP-IDF Environment
# ----------------------------------------------------
def source_idf(idf_export_path, logger):
    logger.info("Setting up ESP-IDF environment...")

    if is_windows():
        # Use export.bat
        if not os.path.isfile(idf_export_path + ".bat"):
            logger.error("export.bat not found!")
            sys.exit(1)

        cmd = f'cmd /c "{idf_export_path}.bat && echo ESP-IDF configured"'
        run_cmd(cmd, logger)

    elif is_unix():
        # Use export.sh
        if not os.path.isfile(idf_export_path + ".sh"):
            logger.error("export.sh not found!")
            sys.exit(1)

        cmd = f"bash -c 'source {idf_export_path}.sh && echo ESP-IDF configured'"
        run_cmd(cmd, logger)

    else:
        logger.error("Unsupported OS")
        sys.exit(1)

# ----------------------------------------------------
#   IDF BUILD / CLEAN
# ----------------------------------------------------
def idf_build(logger, idf_export_path):
    logger.info("Building firmware...")
    
    if is_windows():
        # Run export.bat + build inside SAME cmd.exe
        cmd = f'cmd /c "{idf_export_path}.bat && idf.py build"'
        run_cmd(cmd, logger)
    else:
        run_cmd(f"idf.py build", logger)

def idf_clean(logger, idf_export_path):
    logger.info("Cleaning build directory...")
    
    if is_windows():
        # Run export.bat + build inside SAME cmd.exe
        cmd = f'cmd /c "{idf_export_path}.bat && idf.py fullclean"'
        run_cmd(cmd, logger)
    else:
        run_cmd(f"idf.py fullclean", logger)


def idf_flash(logger, idf_export_path):
    logger.info("Flashing firmware...")
    
    if is_windows():
        # Run export.bat + build inside SAME cmd.exe
        cmd = f'cmd /c "{idf_export_path}.bat && idf.py flash"'
        run_cmd(cmd, logger)
    else:
        run_cmd(f"idf.py flash", logger)

def idf_monitor(logger, idf_export_path):
    logger.info("Monitoring firmware...")
    
    if is_windows():
        # Run export.bat + build inside SAME cmd.exe
        cmd = f'cmd /c "{idf_export_path}.bat && idf.py monitor"'
        run_cmd(cmd, logger)
    else:
        run_cmd(f"idf.py monitor", logger)


    

# ----------------------------------------------------
#   MAIN
# ----------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description="Ridecast Build Utility (Cross Platform)")

    parser.add_argument("-b", "--build", action="store_true", help="Build the project")
    parser.add_argument("-c", "--clean", action="store_true", help="Clean the project")
    parser.add_argument("-f", "--flash", action="store_true", help="Flash the project")
    parser.add_argument("-m", "--monitor", action="store_true", help="Monitor the project")
    parser.add_argument("-l", "--logdebug", action="store_true", help="Enable debug logging")
    parser.add_argument("--all", action="store_true",
                        help="Clean, build, flash and monitor")

    args = parser.parse_args()

    # ---------------- Handle --all EARLY ----------------
    if args.all:
        args.clean = True
        args.build = True
        args.flash = True
        args.monitor = True

    logger = setup_logger(args.logdebug)

    # ---------------- Paths ----------------
    script_root = os.path.dirname(os.path.abspath(__file__))
    sdk_base = os.path.join(script_root, "sdk", "esp")
    idf_export = os.path.join(sdk_base, "export")

    logger.debug(f"SDK path: {sdk_base}")
    logger.debug(f"Export script base: {idf_export}")

    # ---------------- No action selected ----------------
    if not (args.clean or args.build or args.flash or args.monitor):
        logger.warning(
            "No action selected. Use -b, -c, -f, -m or --all."
        )
        return

    # ---------------- Setup ESP-IDF ONCE ----------------
    source_idf(idf_export, logger)

    # ---------------- Actions (ordered) ----------------
    if args.clean:
        idf_clean(logger, idf_export)

    if args.build:
        idf_build(logger, idf_export)

    if args.flash:
        idf_flash(logger, idf_export)

    if args.monitor:
        idf_monitor(logger, idf_export)


if __name__ == "__main__":
    main()
