# Arch Linux package

This directory contains three Arch Linux package recipes that can be published
as separate AUR packages:

- `pboman3` builds a tagged release from source.
- `pboman3-bin` installs the upstream prebuilt AppImage.
- `pboman3-git` builds the latest development revision from Git.

All variants install the GUI, CLI, desktop entry, PBO MIME definition,
application icon, Dolphin service menus, and Nautilus extension.

Install the package from this checkout as a regular user:

```sh
sudo pacman -S --needed base-devel
cd packaging/arch/pboman3
makepkg -si
```

Replace `pboman3` in the path with `pboman3-bin` or `pboman3-git` to install a
different variant. The packages provide and conflict with `pboman3`, so pacman
will prevent two variants from being installed simultaneously.

The `pboman3-git` recipe currently follows `codex/linux-port`, the active
Linux development branch. Change its source fragment to the repository's
default branch when the port is merged there.

Arch expects `base-devel` to be installed before using `makepkg`, so it is not
listed in `makedepends`. `makepkg -s` installs the other required build and
runtime dependencies through pacman.
The context-menu files are included in `pboman3`; install the file manager you
use to activate its integration:

```sh
# KDE Dolphin integration
sudo pacman -S --needed dolphin

# GNOME Files integration
sudo pacman -S --needed nautilus-python
```

`nautilus-python` pulls in Nautilus and its Python/GObject bindings. Close and
reopen Dolphin after installation. Restart Nautilus after installing or
upgrading the package so that it reloads the Python extension.

To remove the Arch package later, use the exact variant that you installed:

```sh
sudo pacman -Rns pboman3
# or: sudo pacman -Rns pboman3-bin
# or: sudo pacman -Rns pboman3-git
```

## AUR maintenance

Each package directory becomes its own AUR Git repository. Copy the directory's
`PKGBUILD` and `.SRCINFO` to the corresponding AUR repository; do not combine
the three recipes into one package base.

For `pboman3` and `pboman3-bin`, update `pkgver`, reset `pkgrel` to `1`, update
the source checksums, and regenerate `.SRCINFO` whenever a new release is
packaged:

```sh
updpkgsums
makepkg --printsrcinfo > .SRCINFO
makepkg --cleanbuild
namcap PKGBUILD pboman3-*.pkg.tar.zst
```

Commit both `PKGBUILD` and `.SRCINFO` to the AUR repository. Regenerate and
commit `.SRCINFO` whenever `PKGBUILD` metadata changes so the AUR web interface
does not show stale package information.
