{ lib, stdenv, linuxPackages, fetchurl }:

let
  version = "2.0.25";
  kernel = linuxPackages.kernel;
  modDirVersion = kernel.modDirVersion;
in

stdenv.mkDerivation {
  name = "cgos-mod-${version}-${kernel.version}";

  src = fetchurl {
    url = "https://git.congatec.com/x86/meta-congatec-x86/-/raw/cc5d6223cf41ed587ef284dce80f2b7ce592e40e/recipes-tools/cgos/files/CGOS_DIRECT_Lx_common_R2.00.0021.tar.bz2";
    sha256 = "sha256-NGRvlVQU9cgcZm7LUUdZJ3U3gEWCdPII1AYE8zT7yCU=";
  };

  patches = [
    ./CGOS_DIRECT_Lx_common_R2.00.0021.patch
    ./CGOS_DIRECT_Lx_common_R2.00.0024.patch
    ./CGOS_DIRECT_Lx_common_R2.00.0025.patch
  ];

  hardeningDisable = [ "pic" "format" ];
  nativeBuildInputs = kernel.moduleBuildDependencies;

  makeFlags = [
    # work around bug in Makefile
    # "KERNELRELEASE=${kernel.modDirVersion}"
    "KERNELDIR=${kernel.dev}/lib/modules/${kernel.modDirVersion}/build"
    "INSTALL_MOD_PATH=$(out)"

    "mod"
  ];

  installPhase = ''
    # mkdir -p $out/lib/modules/${modDirVersion}
    # cp CgosDrv/Lx/cgosdrv.ko $out/lib/modules/${modDirVersion}/cgosdrv.ko
    mkdir -p $out/lib/modules/${modDirVersion}/extra
    cp CgosDrv/Lx/cgosdrv.ko $out/lib/modules/${modDirVersion}/extra/cgosdrv.ko
    # mkdir -p $out/lib/udev/rules.d/
    # cp 99-cgos.rules $out/lib/udev/rules.d/99-cgos.rules
    # mkdir -p $out/lib/modules-load.d/
    # cp cgos.conf $out/lib/modules-load.d/cgos.conf
  '';

  meta = {
    description = "Congatec OS API (CGOS) support";
    homepage = "http://www.congatec.com";
    longDescription = ''
      Congatec OS API (CGOS) support
      Report issues at: https://git.congatec.com/x86/meta-congatec-x86/issues
    '';
    license = lib.licenses.gpl2Only;
    maintainers = []; # TODO: add maintainers
  };
}
