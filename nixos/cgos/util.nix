{ lib, stdenv, fetchurl, cgos }:

stdenv.mkDerivation rec {
  version = "2.0.25";
  name = "cgos-util-${version}";

  src = fetchurl {
    url = "https://git.congatec.com/x86/meta-congatec-x86/-/raw/cc5d6223cf41ed587ef284dce80f2b7ce592e40e/recipes-tools/cgutil/files/cgutillx_158_2.tar";
    sha256 = "sha256-dIAR77araZncVOfU7+7quUSxkX3v4a0BiaY5SdzfetU=";
  };

  patches = [
    ./cgutillx-158-to-161.patch
    ./001-cgutillx-build.patch
  ];

  buildInputs = [ cgos ];

  makeFlags = [ "-C cgutlcmd" ];

  installPhase = ''
    mkdir -p $out/bin
    cp cgutlcmd/cgutlcmd $out/bin
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
