{ pkgs ? import <nixpkgs> {} }:

let
  cgos = pkgs.callPackage ../../nixos/heimchen/nixos/cgos/cgos.nix { };
in
pkgs.mkShell {
  buildInputs = [ pkgs.stdenv pkgs.jansson cgos ];
}
