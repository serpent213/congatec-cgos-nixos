{ lib, pkgs, stdenv, cgos }:

stdenv.mkDerivation rec {
  version = "0.0.1";
  name = "cgos-cgexporter-${version}";

  src = ./.;

  buildInputs = [ cgos pkgs.libmicrohttpd pkgs.libbsd ];

  installPhase = ''
    mkdir -p $out/bin
    cp cgexporter $out/bin
  '';

  meta = {
    description = "Congatec OS API (CGOS) user-land utility";
    homepage = "https://github.com/serpent213/congatec-cgos-nixos";
    license = lib.licenses.publicDomain;
    maintainers = []; # TODO: add maintainers
  };
}
