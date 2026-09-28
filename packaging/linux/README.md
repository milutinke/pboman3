# Linux packaging

The Linux release is built from a single staged AppDir. `package.sh` produces
an AppDir tarball and an AppImage from that directory, then writes SHA-256
checksums for both artifacts.

Build releases on x86-64 Ubuntu 22.04 with glibc 2.35 and without
`-march=native`. Set `LINUXDEPLOY` to the pinned linuxdeploy binary
and place the matching pinned `linuxdeploy-plugin-qt` beside it before running:

```sh
packaging/linux/package.sh build dist
```

The selected Qt installation must provide both the `xcb` and `wayland` platform
plugins. The Qt linuxdeploy plugin also collects the required image and TLS
plugins. Host graphics drivers and the glibc loader must remain unbundled.

Run the AppImage GUI normally. Use its dispatcher for CLI operation:

```sh
./PBOManager-x86_64.AppImage --cli unpack --beside-input -- example.pbo
```

Register or remove the AppImage's GNOME Files, Dolphin, desktop, and MIME
integration for the current user with `--install-integration` and
`--uninstall-integration`. Registration records the stable AppImage path, never
its temporary mount point. Run registration again after moving the AppImage.

The tarball exposes `usr/bin/pbom` and `usr/bin/pboc` directly. Systems that
cannot mount AppImages can extract either the AppImage or the tarball.
