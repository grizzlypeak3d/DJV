# Download and install

[Download the latest release](https://github.com/grizzlypeak3d/DJV/releases/)

The packages require:

- **Linux** — Rocky Linux 8 or newer (glibc 2.28), x86-64
- **Windows** — Windows 10 or newer, 64-bit
- **macOS** — macOS 11 or newer on Apple silicon, or macOS 10.15 or newer on Intel
- A graphics driver with OpenGL 4.1, or OpenGL ES 3

Other systems can build from source; see the [README](https://github.com/grizzlypeak3d/DJV).

> [!NOTE]
> Download packages include only a minimal set of video and audio codecs. To support additional codecs, install FFmpeg and DJV will use it for the files it cannot decode itself (see [Files](files.html)), or build from source.

## Linux

Packages are distributed as tar archives. Uncompress the archive and move the DJV folder to a convenient location.

The archive has a desktop entry and icons under `share`, for the applications menu and for opening files from the file manager. The entry runs the application from the path; to install it for the current user, point it at the copy and put it where the desktop looks:

```
cd DJV
for entry in share/applications/*.desktop; do
    sed "s|^Exec=|Exec=$PWD/bin/|" $entry > ~/.local/share/applications/$(basename $entry)
done
cp -r share/icons ~/.local/share/
update-desktop-database ~/.local/share/applications
```

## Windows

Packages are distributed as NSIS installers or ZIP archives. Double-click the NSIS `.exe` file to start the installer, or, for ZIP archives, uncompress the archive and move the DJV folder to a convenient location.

## macOS

Packages are distributed as macOS disk images, one for Apple silicon (`arm64`) and one for Intel (`x86_64`). Open the disk image and copy DJV to the **Applications** folder.

## pip

DJV can also be installed with pip, for Python 3.12 or newer on Linux, macOS, and Windows:

```
pip install DJV-viewer
djv
```

The `djv` command runs the application. This is also the way to install on systems the packages are not built for, such as Linux on ARM, a Raspberry Pi 5 among them. The same codecs are included as in the packages.

## VFX Reference Platform {#vfx-platform}

DJV builds and bundles its own libraries, so it runs whatever versions a facility has installed. What a pipeline touches is the Python modules and the color and image libraries that a show's files are made with. This release against the [VFX Reference Platform](https://vfxplatform.com/):

| Component | DJV | CY2026 | CY2027 |
|---|---|---|---|
| Python | 3.12 or newer | 3.13 | 3.13 |
| C++ | C++20 | C++20 | C++20 |
| OpenColorIO | 2.5 | 2.5 | 2.6 |
| OpenEXR | 3.4 | 3.4 | 3.5 |
| Imath | 3.2 | 3.2 | 3.2 |
| Linux (glibc) | 2.28 or newer | 2.28 | 2.34 |
| macOS | 11 or newer (10.15 on Intel) | 14 | 14 |

DJV matches CY2026. The Python modules are built for Python's stable ABI, so one install works with 3.12, 3.13 and newer.

The bindings DJV is built from are importable as `djv`, alongside `tlrender` and `feather_tk`; see [Python](python.html).

> [!NOTE]
> Python 3.12 or 3.13 is the easy choice for now. The bindings depend on OpenTimelineIO, which has no prebuilt wheels yet for Python 3.14, or for Intel macOS: there pip builds it from source, which needs a C++ compiler and the Python development files (`python3-dev` on Debian and Ubuntu, `python3-devel` on Fedora and Rocky).
