# RideCast Docker toolchain — ESP-IDF 5.5

The **entire Docker build setup lives under `docker/`**. Firmware source code lives in the repository on the host and is bind-mounted read/write into the container. Generated outputs live in `build-docker/` at repository root, separate from `build/` used by native builds. No `.bat`, `.ps1`, or `.sh` build wrappers are needed.

## Requirements

- Windows: Docker Desktop using Linux containers + Python 3.9+ on host.
- Linux: Docker Engine and Docker Compose plugin + Python 3.9+ on host.
- Docker service access and disk space for Espressif's ESP-IDF image.
- Git submodule checked out at **ESP-IDF 5.5.x** (the toolchain image uses `espressif/idf:v5.5`).

Initialize your SDK submodule from repository root:

```bash
git submodule update --init --recursive sdk/esp
```

## Install (at repository root)

```bash
python docker/build.py --image-build
python docker/build.py --build
```

On Windows substitute `py` for `python` if needed; on Linux `python3` is also accepted.

## Everyday commands

```bash
python docker/build.py --build
python docker/build.py --fullclean --build
python docker/build.py --clean
python docker/build.py --menuconfig
python docker/build.py --size
```

## Flashing on Linux

The serial device is passed to the container using `--device`:

```bash
python3 docker/build.py --flash --port /dev/ttyUSB0
python3 docker/build.py --monitor --port /dev/ttyUSB0
```

Ensure that your user has permissions on the device and it is not held by another terminal. If needed, add your user to the `dialout` group and re-login. Docker cannot flash a device that is not available on the host.

## Flashing on Windows

Docker Desktop Linux containers normally cannot access a Windows `COM` port directly. The **Docker build output is flashed with host Python/esptool**; the host does not need ESP-IDF, CMake, Ninja, or a C compiler.

```powershell
py -m pip install -r docker/requirements-host-flash.txt
py docker/build.py --build
py docker/build.py --flash --port COM7
py docker/build.py --monitor --port COM7
```

The host-side flash code reads `build-docker/flasher_args.json` produced by ESP-IDF, respecting the actual firmware offsets rather than hardcoding bootloader/partition addresses.

If you require every flashing operation to take place in Docker on Windows too, USB pass-through using WSL2/usbipd-win is an optional separate hardware configuration. It is **not** provided or tested by this starter.

## Project integration notes

- `docker/container.py` sets `IDF_PATH` to **your mounted `sdk/esp` submodule**, so ESP-IDF component discovery uses the same SDK checkout as native builds.
- The Docker image supplies the already-installed ESP-IDF **5.5** build tools and Python environment; pin the image digest for strictly reproducible CI builds.
- Your submodule should use compatible ESP-IDF 5.5 versions and dependencies. If it has been modified, missing dependencies or a different patch release may require rebuilding the image/tool environment.
- The generated directory `build-docker/` should be added to the **repository root `.gitignore`** (not modified by this package).
- `menuconfig` requires an interactive terminal. On noninteractive CI use `sdkconfig.defaults` instead.
- Linux builds run under your host UID/GID so output files aren't root-owned. Some Docker configurations may require additional host permissions.
- The original root `build.py` is unchanged, and native `build/` is never used by this Docker runner.

## Files

| File | Purpose |
| --- | --- |
| `Dockerfile` | Official ESP-IDF 5.5 toolchain as a reusable image |
| `compose.yaml` | Defines container and source-code bind mount |
| `build.py` | Cross-platform user command-line interface |
| `container.py` | Runs `idf.py` inside Docker with project SDK checkout |
| `requirements-host-flash.txt` | Windows host Python serial-flashing dependencies |
| `.dockerignore` | Keep Docker image context minimal |

## Troubleshooting

- `sdk/esp` missing: `git submodule update --init --recursive sdk/esp`.
- Docker not running: launch Docker Desktop / start Docker Engine.
- Windows module `esptool` missing: install `docker/requirements-host-flash.txt`.
- `flasher_args.json` missing: build firmware first.
- Build fails in `idf_component_register`: inspect the project's component CMakeLists; Docker does not automatically fix ESP-IDF component discovery problems.
- Linux USB permissions: ensure your user has access to the serial device; stop other serial monitors.

## Verification status

Python syntax was checked in the creation environment; a live Docker image build, full RideCast compilation, and hardware flashing still need to be validated on your local machine.
