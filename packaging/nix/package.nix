{
  lib,
  stdenv,
  cmake,
  ninja,
  cli11,
  qt6,
  xdg-utils,
  src ? ../..,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "pboman3";
  version = "1.11.0";

  inherit src;

  strictDeps = true;

  nativeBuildInputs = [
    cmake
    ninja
    qt6.wrapQtAppsHook
  ];

  buildInputs = [
    cli11
    qt6.qtbase
    qt6.qtwayland
  ];

  cmakeFlags = [
    (lib.cmakeBool "BUILD_TESTING" false)
    (lib.cmakeBool "PBOM_BUILD_GUI" true)
    (lib.cmakeBool "PBOM_BUILD_CLI" true)
    (lib.cmakeBool "PBOM_USE_SYSTEM_CLI11" true)
    (lib.cmakeFeature "PBOM_VERSION" finalAttrs.version)
  ];

  postInstall = ''
    install -Dm644 LICENSE "$out/share/licenses/pboman3/LICENSE"
  '';

  qtWrapperArgs = [
    "--prefix PATH : ${lib.makeBinPath [ xdg-utils ]}"
  ];

  meta = {
    description = "GUI and command-line tools for packing and unpacking Arma PBO files";
    homepage = "https://github.com/winseros/pboman3";
    license = lib.licenses.agpl3Plus;
    mainProgram = "pbom";
    platforms = lib.platforms.linux;
  };
})
