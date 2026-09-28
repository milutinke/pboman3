# Linux desktop integration

PBO Manager ships standard MIME and desktop metadata, Dolphin service menus,
and a launch-only Nautilus extension. All archive operations remain in `pbom`.

For a portable installation, register a stable absolute path. Do not register
an executable below an AppImage temporary mount directory:

```sh
./linux/install-user.sh --launcher "$PWD/AppDir/usr/bin/pbom"
```

Running the installer again updates the recorded path and all generated desktop
files. Remove only PBO Manager's per-user integration with:

```sh
./linux/uninstall-user.sh
```

Nautilus requires the distribution package commonly named `nautilus-python`.
Restart Nautilus after installing or removing the extension. Dolphin reads its
service menus from the standard `kio/servicemenus` data directory.

The context menus accept local, homogeneous selections only. A directory
selection receives pack actions; a `.pbo` file selection receives unpack
actions. Mixed selections and remote URLs receive no PBO Manager actions.

