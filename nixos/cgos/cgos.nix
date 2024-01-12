{ lib, stdenv, fetchurl }:

stdenv.mkDerivation rec {
  version = "2.0.25";
  name = "cgos-${version}";

  src = fetchurl {
    url = "https://git.congatec.com/x86/meta-congatec-x86/-/raw/cc5d6223cf41ed587ef284dce80f2b7ce592e40e/recipes-tools/cgos/files/CGOS_DIRECT_Lx_common_R2.00.0021.tar.bz2";
    sha256 = "sha256-NGRvlVQU9cgcZm7LUUdZJ3U3gEWCdPII1AYE8zT7yCU=";
  };

  patches = [
    ./CGOS_DIRECT_Lx_common_R2.00.0021.patch
    ./CGOS_DIRECT_Lx_common_R2.00.0024.patch
    ./CGOS_DIRECT_Lx_common_R2.00.0025.patch
  ];

  makeFlags = [ "app" ];

  installPhase = ''
    mkdir -p $out/include
    cp CgosLib/Cgos.h $out/include
    mkdir -p $out/lib
    cp CgosLib/Lx/libcgos.so $out/lib
    mkdir -p $out/bin
    cp CgosDump/Lx/cgosdump $out/bin
    cp CgosMon/Lx/cgosmon $out/bin
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
