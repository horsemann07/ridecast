#!/usr/bin/env python3
"""Cross-platform Docker interface for RideCast. Requires only Python + Docker.

Usage:
    python docker/build.py --build
    python docker/build.py --fullclean --build
    python docker/build.py --flash --port COM7
    python docker/build.py --flash --port /dev/ttyUSB0
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

DOCKER_DIR = Path(__file__).resolve().parent
ROOT = DOCKER_DIR.parent
COMPOSE = DOCKER_DIR / 'compose.yaml'
OUTPUT = ROOT / 'build-docker'


def execute(command: list[str], *, cwd: Path = ROOT) -> None:
    print('+', ' '.join(command), flush=True)
    subprocess.run(command, cwd=cwd, check=True)


def in_docker(action: str, port: str | None = None, baud: int = 460800) -> None:
    command = [
        'docker', 'compose', '-f', str(COMPOSE), 'run', '--rm', '--no-deps'
    ]
    if platform.system() == 'Linux':
        # Prevent root-owned objects inside the host-mounted build-docker directory.
        command += ['--user', f'{os.getuid()}:{os.getgid()}', '-e', 'HOME=/tmp']
        if action in ('flash', 'monitor'):
            if not port or not port.startswith('/dev/'):
                raise ValueError('Linux Docker flash/monitor requires /dev/ttyUSB* or /dev/ttyACM* port')
            command += ['--device', f'{port}:{port}', '--group-add', str(os.stat(port).st_gid)]
    command += ['ridecast', 'python3', 'docker/container.py', action]
    if port is not None:
        command += ['--port', port]
    if action in ('flash', 'monitor'):
        command += ['--baud', str(baud)]
    execute(command)


def host_flash(port: str, baud: int) -> None:
    """Flash Windows serial ports using the firmware's generated ESP-IDF metadata."""
    manifest = OUTPUT / 'flasher_args.json'
    if not manifest.is_file():
        raise FileNotFoundError('Missing build-docker/flasher_args.json; build firmware first')
    configuration = json.loads(manifest.read_text(encoding='utf-8'))
    settings = configuration.get('extra_esptool_args', {})
    chip = settings.get('chip', 'esp32')
    before = settings.get('before', 'default_reset')
    after = settings.get('after', 'hard_reset')
    write_args = configuration.get('write_flash_args', [])
    flash_files = configuration.get('flash_files', {})
    if not isinstance(write_args, list) or not isinstance(flash_files, dict) or not flash_files:
        raise ValueError('Invalid flasher_args.json flash configuration')
    command = [sys.executable, '-m', 'esptool', '--chip', str(chip), '--port', port,
               '--baud', str(baud), '--before', str(before), '--after', str(after)]
    if settings.get('stub') is False:
        command.append('--no-stub')
    command += ['write_flash', *[str(item) for item in write_args]]
    for address, filename in flash_files.items():
        path = (OUTPUT / filename).resolve()
        if not path.is_relative_to(OUTPUT.resolve()) or not path.is_file():
            raise FileNotFoundError(f'Invalid or missing flash image: {filename}')
        command += [str(address), str(path)]
    execute(command)


def main() -> int:
    parser = argparse.ArgumentParser(description='RideCast ESP-IDF 5.5 Docker build/flash')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--action', choices=('build', 'clean', 'fullclean', 'menuconfig', 'flash', 'monitor', 'size'))
    parser.add_argument('-b', '--build', action='store_true')
    parser.add_argument('-c', '--clean', action='store_true')
    parser.add_argument('-fc', '--fullclean', action='store_true')
    parser.add_argument('-f', '--flash', action='store_true')
    parser.add_argument('-m', '--monitor', action='store_true')
    parser.add_argument('--menuconfig', action='store_true')
    parser.add_argument('--size', action='store_true')
    parser.add_argument('-p', '--port', help='COM7 (Windows) or /dev/ttyUSB0 (Linux)')
    parser.add_argument('--baud', type=int, default=460800)
    parser.add_argument('--image-build', action='store_true', help='Build/rebuild toolchain image')
    args = parser.parse_args()
    tasks = []
    if args.action:
        tasks.append(args.action)
    else:
        for enabled, name in ((args.fullclean, 'fullclean'), (args.clean, 'clean'),
                              (args.menuconfig, 'menuconfig'), (args.build, 'build'),
                              (args.size, 'size'), (args.flash, 'flash'),
                              (args.monitor, 'monitor')):
            if enabled:
                tasks.append(name)
    if 'fullclean' in tasks and 'clean' in tasks:
        parser.error('Use either --clean or --fullclean')
    if not tasks and not args.image_build:
        parser.error('Select --build, --flash, --fullclean, --menuconfig, etc.')
    if any(item in ('flash', 'monitor') for item in tasks) and not args.port:
        parser.error('--port is required for --flash or --monitor')

    try:
        if args.image_build:
            execute(['docker', 'compose', '-f', str(COMPOSE), 'build', 'ridecast'])
        for task in tasks:
            if task == 'flash' and platform.system() == 'Windows':
                host_flash(args.port, args.baud)
            elif task == 'monitor' and platform.system() == 'Windows':
                execute([sys.executable, '-m', 'serial.tools.miniterm', args.port, '115200'])
            else:
                in_docker(task, args.port if task in ('flash', 'monitor') else None, args.baud)
    except (ValueError, FileNotFoundError, subprocess.CalledProcessError, OSError) as exc:
        print(f'[RideCast Docker] {exc}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
