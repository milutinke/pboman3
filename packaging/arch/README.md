# Arch Linux package

This directory contains the Arch Linux package recipe that can later be
published as the `pboman3` AUR package. It builds the tagged source release and
installs the GUI, CLI, desktop entry, PBO MIME definition, application icon,
Dolphin service menus, and Nautilus extension under `/usr`.

Install the package from this checkout as a regular user:

```sh
sudo pacman -S --needed base-devel
cd packaging/arch
makepkg -si
```

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

To remove the Arch package later:

```sh
sudo pacman -Rns pboman3
```

## AUR maintenance

Update `pkgver`, reset `pkgrel` to `1`, update the source checksums, and then
regenerate `.SRCINFO` whenever a new upstream release is packaged:

```sh
updpkgsums
makepkg --printsrcinfo > .SRCINFO
makepkg --cleanbuild
namcap PKGBUILD pboman3-*.pkg.tar.zst
```

Commit both `PKGBUILD` and `.SRCINFO` to the AUR repository. Regenerate and
commit `.SRCINFO` whenever `PKGBUILD` metadata changes so the AUR web interface
does not show stale package information.
