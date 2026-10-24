# ============================================================================
# File        : Dockerfile
# Description : RideCast ESP-IDF 5.5 build toolchain (no firmware source copied)
# Date        : 2026-10-24
# ============================================================================
# Copyright (c) 2026 Navyantra Tech Private Limited
# ============================================================================
# Change Log (newest first):
# 2026-10-24  Initial containerized toolchain version.
# ============================================================================

# Pin a major.minor release image; pin a digest in CI for byte-for-byte toolchain reproducibility.
FROM espressif/idf:v5.5

WORKDIR /workspace/ridecast
ENV IDF_CCACHE_ENABLE=0

# The repository is bind-mounted at runtime. No COPY or duplicate SDK install.
