#!/usr/bin/env bash
# CI uses ubuntu-24.04 (Noble), including the GCC 13 toolchain used by the tests.
set -euo pipefail

packages=(binutils-arm-none-eabi gcc-arm-none-eabi libnewlib-arm-none-eabi libpng-dev python3 "$@")
apt_dir=$(mktemp -d)
trap 'rm -rf "$apt_dir"' EXIT
chmod 755 "$apt_dir"
apt_options=(
    -o "Dir::Etc::sourcelist=$apt_dir/sources.list"
    -o Dir::Etc::sourceparts=-
    -o APT::Update::Error-Mode=any
    -o Acquire::Retries=2
    -o Acquire::http::Timeout=30
    -o Acquire::https::Timeout=30
    -o DPkg::Lock::Timeout=60
)

# #1474 spent almost 44 minutes downloading from the Azure mirror. Prefer the
# main archive, then try Azure once if that endpoint is unavailable or slow.
# Scope sources to this invocation; keep Ubuntu signature verification enabled.
for mirror in https://archive.ubuntu.com/ubuntu https://azure.archive.ubuntu.com/ubuntu; do
    : > "$apt_dir/sources.list"
    for suite in noble noble-updates noble-backports noble-security; do
        printf 'deb [signed-by=/usr/share/keyrings/ubuntu-archive-keyring.gpg] %s %s main restricted universe multiverse\n' \
            "$mirror" "$suite" >> "$apt_dir/sources.list"
    done
    echo "Downloading CI dependencies from $mirror (90s metadata, 180s packages)"
    if sudo env DEBIAN_FRONTEND=noninteractive timeout --verbose --kill-after=15s 90s \
        apt-get "${apt_options[@]}" update \
        && sudo env DEBIAN_FRONTEND=noninteractive timeout --verbose --kill-after=15s 180s \
        apt-get "${apt_options[@]}" -y --no-install-recommends --download-only install "${packages[@]}"; then
        # Only downloads are retried/interrupted. Once all archives are ready,
        # install from cache so dpkg is not interrupted by the download timeout.
        sudo env DEBIAN_FRONTEND=noninteractive NEEDRESTART_MODE=a \
            apt-get "${apt_options[@]}" -y --no-install-recommends --no-download install "${packages[@]}"
        exit 0
    fi
    echo "::warning::Dependency download failed or timed out at $mirror"
done

echo "::error::Both Ubuntu mirrors failed; no dependency installation was attempted."
exit 1
