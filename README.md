# PBO Manager

A tool to open, pack and unpack ArmA PBO files.

## Key features
 - Can pack/unpack PBO files.
 - Can preview files inside a PBO.
 - Can open mangled (by Mikero's tools) PBO.
 - Integrates with Windows Explorer / immersive menu, GNOME Files, and KDE Dolphin.
 - Supports PBO metadata provisioning through [pbo.json](doc/pbo_json.md) or [\$PBOPREFIX\$](doc/prefix_files.md) files.

## Screenshots

![Main application window](doc/img/screenshot01.png 'Main application window')

### Context menu integration

![Unpack PBO context menu](doc/img/screenshot02.png 'Unpack PBO context menu')
![Pack folder as PBO context menu](doc/img/screenshot03.png 'Pack folder as PBO context menu')

### Windows 11 immersive menu integration

![Windows 11 immersive menu](doc/img/screenshot04.png 'Windows 11 immersive menu integration')

## Installation 

:small_blue_diamond: The application requires [Microsoft Visual C++ Redistributable 2015-2019](https://aka.ms/vs/16/release/vc_redist.x64.exe) to run. Othervise, the application will complain regarding the `vcruntime140.dll`.

### Windows 10

#### Installer
1. Download the `installer` from the [Releases](https://github.com/winseros/pboman3/releases) section.
2. Run the `installer`.

#### Manual instllation
1. Download `PBOManager-Windows-x86_64.zip` from the [Releases](https://github.com/winseros/pboman3/releases) section and extract it.
2. Run the `pbom.exe`
3. Optionally, to get the Windows Explorer integration, register the `dll`:
   ```
    C:\Windows\SysWOW64\regsvr32.exe C:\Where\The\App\Is\pboe.dll
   ```
   To unregister the `dll` later, use:
   ```
   C:\Windows\SysWOW64\regsvr32.exe /u C:\Where\The\App\Is\pboe.dll
   ```
   Normally no admin permissions should be required for the registration.

## Windows 11

The [Windows 10](#windows-10) installation instructions are still valid for Windows 11, although the MSIX package is the suggested method of installation. The Windows 11 immersive menu integration won't function otherwise. :pushpin:

## Linux

Linux releases target x86-64 distributions with glibc 2.35 or newer. Download
either the AppImage or the AppDir tarball from the Releases page. The AppImage
includes a CLI dispatcher:

```sh
chmod +x PBOManager-x86_64.AppImage
./PBOManager-x86_64.AppImage
./PBOManager-x86_64.AppImage --cli --help
./PBOManager-x86_64.AppImage --install-integration
```

The AppDir tarball exposes `usr/bin/pbom` and `usr/bin/pboc`. It is also the
fallback on systems where AppImage/FUSE mounting is unavailable.

GNOME Files and KDE Dolphin integration can be installed for the current user
with the AppImage command above. Remove it with `--uninstall-integration`.

For an unpacked AppDir, run its integration helper and pass a stable absolute
path. Do not pass a path below an AppImage temporary mount:

```sh
./usr/share/pboman3/integration/install-user.sh \
  --launcher "$PWD/usr/bin/pbom"
```

Run `uninstall-user.sh` from the same directory to remove these entries. GNOME
Files requires the host distribution's `nautilus-python` package and must be
restarted after an integration change. Other file managers receive the standard
`application/x-pbo` MIME registration and can use **Open With PBO Manager**.

The dedicated menus appear only for local homogeneous selections. They provide
**Pack to…**, **Pack beside folder(s)**, **Unpack to…**, and
**Unpack beside archive(s)**. Remote URLs and mixed file/folder selections are
left unchanged.

## Building from source

PBO Manager requires CMake, a C++20 compiler, and Qt 6.8 or newer. Windows
release builds use Qt 6.10.1; Linux release builds use Qt 6.8.3 because its
official binary package includes both native xcb and Wayland platform plugins.
Linux builds require the Qt Widgets, Network, xcb, and Wayland components
supplied by the Qt installation.

1. Set the env variables:

   | Variable | Description                                                       | Example                         |
   |----------|-------------------------------------------------------------------|---------------------------------|
   | Qt6_ROOT | Where Qt is located. Needed for CMake to build.                   | G:\Qt\6.10.1\msvc2022_64        |


2. Run the script:
   
   ```
   # powershell
   git clone --recurse-submodules git@github.com:winseros/pboman3.git
   cmake -S <path_to_source_code> -B <path_to_build_files>
   cmake --build <path_to_build_files>
   ```

Also, see [how CI builds](.github/workflows/artifcats.yaml).

On Linux, a typical source build is:

### Arch Linux package

Arch Linux users can build and install a native pacman package from the
included AUR-ready recipe:

```sh
sudo pacman -S --needed base-devel
git clone https://github.com/milutinke/pboman3.git
cd pboman3/packaging/arch
makepkg -si
```

The package owns the GUI, CLI, desktop entry, PBO MIME definition, icon, and
the integration files for both Dolphin and GNOME Files. File managers remain
optional dependencies so installing PBO Manager does not install two desktop
stacks. Install the integration for the file manager you use:

```sh
sudo pacman -S --needed dolphin          # KDE Dolphin context menus
sudo pacman -S --needed nautilus-python  # GNOME Files context menus
```

Close and reopen Dolphin after installation. Restart Nautilus so it reloads
the Python extension. See [the Arch packaging notes](packaging/arch/README.md)
for package removal and future AUR maintenance.

### Linux build and install script

Install the required build tools, a C++20 compiler, Qt 6.8 or newer, and the Qt
Widgets, Network, xcb, and Wayland development components. GNOME Files context
menus additionally require the distribution's `nautilus-python` package.

Clone with submodules and run the included installer:

```sh
git clone --recurse-submodules https://github.com/winseros/pboman3.git
cd pboman3
./linux/build-install.sh
```

The default installation prefix is `$HOME/.local`, so root access is not
required. The script builds both `pbom` and `pboc`, installs the desktop entry,
MIME definition, icon, Dolphin service menus, and Nautilus extension, and then
refreshes the available desktop caches. Ensure `$HOME/.local/bin` is in `PATH`
and restart Nautilus or Dolphin after installation.

Useful options include:

```sh
# Rebuild from scratch with eight parallel jobs
./linux/build-install.sh --clean --jobs 8

# Install system-wide under /usr/local; sudo is used only for installation
./linux/build-install.sh --system

# Select a different prefix and build directory
./linux/build-install.sh \
  --prefix "$HOME/Applications/pboman3" \
  --build-dir "$PWD/build/custom"

./linux/build-install.sh --help
```

### Manual Linux build

The equivalent manual build is:

```sh
git clone --recurse-submodules https://github.com/winseros/pboman3.git
cmake -S pboman3 -B pboman3/build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build pboman3/build
cmake --install pboman3/build
```

Native installation places the desktop file, MIME definition, Dolphin service
menus, and Nautilus extension in standard GNU install directories. Distribution
packages should refresh their desktop, MIME, and icon caches in package hooks.
See [Linux integration](linux/README.md) and
[Linux packaging](packaging/linux/README.md) for portable registration and
release details. Linux packaging intentionally leaves host graphics drivers and
the glibc loader outside the bundle.

## Open in IDE

1. Set the env variabls:

   | Variable | Description                                                       | Example                         |
   |----------|-------------------------------------------------------------------|---------------------------------|
   | Qt6_ROOT | Where QT is located. Needed for CMAKE to build.                   | G:\Qt\6.3.0\msvc2019_64         |
   | PATH     | Where QT binaries are located. Needed for the IDE to run/debug.   | G:\Qt\6.3.0\msvc2019_64\bin     |
   | PATH     | Where OpenSSL binaries are located. Needed for IDE to run/debug.  | G:\Qt\Tools\OpenSSL\Win_x64\bin |

2. Open the the root folder in IDE
