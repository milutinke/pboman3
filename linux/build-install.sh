#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)

prefix=${HOME:+$HOME/.local}
prefix=${prefix:-/usr/local}
build_dir=$source_dir/build/release
build_type=Release
jobs=
clean=0
system_install=0
prefix_explicit=0
initialize_submodules=1
refresh_caches=1

usage() {
    cat <<EOF
Usage: $0 [OPTIONS]

Build and install the PBO Manager GUI, CLI, and Linux desktop integration.

Options:
  --prefix PATH          Installation prefix (default: \$HOME/.local)
  --build-dir PATH       CMake build directory (default: build/release)
  --build-type TYPE      CMake build type (default: Release)
  --jobs COUNT           Maximum parallel build jobs
  --clean                Remove the selected build directory first
  --system               Install under /usr/local using sudo
  --skip-submodules      Do not initialize missing Git submodules
  --no-cache-refresh     Do not refresh desktop, MIME, and icon caches
  -h, --help             Show this help
EOF
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --prefix)
            [ "$#" -ge 2 ] || { usage >&2; exit 2; }
            prefix=$2
            prefix_explicit=1
            shift 2
            ;;
        --build-dir)
            [ "$#" -ge 2 ] || { usage >&2; exit 2; }
            build_dir=$2
            shift 2
            ;;
        --build-type)
            [ "$#" -ge 2 ] || { usage >&2; exit 2; }
            build_type=$2
            shift 2
            ;;
        --jobs)
            [ "$#" -ge 2 ] || { usage >&2; exit 2; }
            jobs=$2
            shift 2
            ;;
        --clean)
            clean=1
            shift
            ;;
        --system)
            system_install=1
            shift
            ;;
        --skip-submodules)
            initialize_submodules=0
            shift
            ;;
        --no-cache-refresh)
            refresh_caches=0
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            printf '%s\n' "Unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [ "$system_install" -eq 1 ] && [ "$prefix_explicit" -eq 0 ]; then
    prefix=/usr/local
fi

case "$prefix" in
    /*) ;;
    *) printf '%s\n' "Installation prefix must be an absolute path: $prefix" >&2; exit 2 ;;
esac

case "$build_dir" in
    /*) ;;
    *) build_dir=$source_dir/$build_dir ;;
esac

for tool in cmake git; do
    command -v "$tool" >/dev/null 2>&1 || {
        printf '%s\n' "Required command is not installed: $tool" >&2
        exit 1
    }
done

if [ "$initialize_submodules" -eq 1 ] && [ ! -f "$source_dir/__lib__/cli11/CMakeLists.txt" ]; then
    printf '%s\n' "Initializing pinned Git submodules..."
    git -C "$source_dir" submodule update --init --recursive
fi

if [ "$clean" -eq 1 ]; then
    case "$build_dir" in
        ""|/) printf '%s\n' "Refusing to clean unsafe build directory: $build_dir" >&2; exit 2 ;;
    esac
    rm -rf -- "$build_dir"
fi

printf '%s\n' "Configuring PBO Manager ($build_type)..."
if command -v ninja >/dev/null 2>&1; then
    cmake -S "$source_dir" -B "$build_dir" -G Ninja \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DBUILD_TESTING=OFF \
        -DPBOM_BUILD_GUI=ON \
        -DPBOM_BUILD_CLI=ON
else
    cmake -S "$source_dir" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DBUILD_TESTING=OFF \
        -DPBOM_BUILD_GUI=ON \
        -DPBOM_BUILD_CLI=ON
fi

printf '%s\n' "Building PBO Manager..."
if [ -n "$jobs" ]; then
    cmake --build "$build_dir" --parallel "$jobs"
else
    cmake --build "$build_dir" --parallel
fi

printf '%s\n' "Installing PBO Manager to $prefix..."
if [ "$system_install" -eq 1 ]; then
    command -v sudo >/dev/null 2>&1 || {
        printf '%s\n' "The --system option requires sudo." >&2
        exit 1
    }
    sudo cmake --install "$build_dir"
else
    cmake --install "$build_dir"
fi

for executable in pbom pboc; do
    [ -x "$prefix/bin/$executable" ] || {
        printf '%s\n' "Installed executable is missing: $prefix/bin/$executable" >&2
        exit 1
    }
done

if [ "$refresh_caches" -eq 1 ]; then
    data_dir=$prefix/share
    if [ "$system_install" -eq 1 ]; then
        command -v update-mime-database >/dev/null 2>&1 && sudo update-mime-database "$data_dir/mime"
        command -v update-desktop-database >/dev/null 2>&1 && sudo update-desktop-database "$data_dir/applications"
        command -v gtk-update-icon-cache >/dev/null 2>&1 && \
            sudo gtk-update-icon-cache -f -t "$data_dir/icons/hicolor" >/dev/null 2>&1 || true
    else
        command -v update-mime-database >/dev/null 2>&1 && update-mime-database "$data_dir/mime"
        command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$data_dir/applications"
        command -v gtk-update-icon-cache >/dev/null 2>&1 && \
            gtk-update-icon-cache -f -t "$data_dir/icons/hicolor" >/dev/null 2>&1 || true
    fi
fi

cat <<EOF

PBO Manager is installed.
  GUI: $prefix/bin/pbom
  CLI: $prefix/bin/pboc

Add $prefix/bin to PATH. Restart your file manager to reload its menus.
Install nautilus-python if you use GNOME Files.
EOF
