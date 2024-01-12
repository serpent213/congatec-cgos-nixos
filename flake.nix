{
  description = "NixOS flake for EFCO U7-130";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.11";
  };

  outputs = { self, nixpkgs, ... }: {
    nixosConfigurations.conga = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";

      modules = [
        {
          nix = {
            settings.experimental-features = [ "nix-command" "flakes" ];
          };
        }
        ./nixos/configuration.nix
      ];
    };
  };
}
