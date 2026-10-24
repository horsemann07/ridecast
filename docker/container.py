#!/usr/bin/env python3
"""Run inside Espressif's image. Only used by docker/build.py."""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path('/workspace/ridecast')
ACTIONS = ('build', 'clean', 'fullclean', 'menuconfig', 'flash', 'monitor', 'size')


def main() -> int:
    parser = argparse.ArgumentParser(description='RideCast container ESP-IDF runner')
    parser.add_argument('action', choices=ACTIONS)
    parser.add_argument('--port', default=None)
    parser.add_argument('--baud', type=int, default=460800)
    args = parser.parse_args()

    sdk = ROOT / 'sdk' / 'esp'
    project_file = sdk / 'tools' / 'cmake' / 'project.cmake'
    if not project_file.is_file():
        parser.error('sdk/esp must be initialized: git submodule update --init --recursive')

    version_file = sdk / 'version.txt'
    if version_file.is_file():
        version = version_file.read_text(encoding='utf-8').strip()
        if not version.startswith('5.5'):
            parser.error(f'sdk/esp version {version!r} is not ESP-IDF 5.5; rebuild with a matching image')

    environment = os.environ.copy()
    environment['IDF_PATH'] = str(sdk)
    environment['IDF_CCACHE_ENABLE'] = '0'
    # Image entrypoint already exports compiler/tool/Python paths for its 5.5 installation.
    cmd = ['idf.py', '-B', 'build-docker']
    if args.action in ('flash', 'monitor'):
        if not args.port:
            parser.error(f'{args.action} requires --port')
        cmd += ['-p', args.port, '-b', str(args.baud)]
    cmd.append(args.action)
    print('Running:', ' '.join(cmd), flush=True)
    return subprocess.run(cmd, cwd=ROOT, env=environment, check=False).returncode


if __name__ == '__main__':
    sys.exit(main())
