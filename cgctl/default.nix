{ lib, pkgs, stdenv, cgos }:

stdenv.mkDerivation rec {
  version = "0.0.1";
  name = "cgos-cgctl-${version}";

  src = ./.;

  buildInputs = [ cgos pkgs.jansson ];

  installPhase = ''
    mkdir -p $out/bin
    cp cgctl $out/bin
  '';

  meta = {
    description = "Congatec OS API (CGOS) user-land utility";
    maintainers = []; # TODO: add maintainers
  };
}
