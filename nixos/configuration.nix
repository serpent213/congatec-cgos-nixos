# Edit this configuration file to define what should be installed on
# your system. Help is available in the configuration.nix(5) man page, on
# https://search.nixos.org/options and in the NixOS manual (`nixos-help`).

{ config, lib, pkgs, ... }:
let
  cgos-mod = config.boot.kernelPackages.callPackage ./cgos/kmod.nix {};
  cgos = pkgs.callPackage ./cgos/cgos.nix {};
  cgos-util = pkgs.callPackage ./cgos/util.nix { inherit cgos; };
  cgos-cgctl = pkgs.callPackage ../cgctl { inherit cgos; };
  cgos-cgexporter = pkgs.callPackage ../cgexporter { inherit cgos; };
in
{
  imports =
    [ # Include the results of the hardware scan.
      ./hardware-configuration.nix
    ];

  # Use the systemd-boot EFI boot loader.
  boot.loader.systemd-boot.enable = true;
  boot.loader.efi.canTouchEfiVariables = false;

  boot.kernelModules = [ "cgosdrv" ];
  boot.extraModulePackages = [ cgos-mod ];

  networking = {
    hostName = "conga";
  };

  # Set your time zone.
  time.timeZone = "Europe/Berlin";

  # Select internationalisation properties.
  i18n.defaultLocale = "en_GB.UTF-8";
  # console = {
  #   font = "Lat2-Terminus16";
  #   keyMap = "us";
  #   useXkbConfig = true; # use xkb.options in tty.
  # };

  # List packages installed in system profile. To search, run:
  # $ nix search wget
  environment.systemPackages = (with pkgs; [
    asciiquarium
    curl
    htop
    git
    neovim
    pciutils
    tcpdump
    tmux
    usbutils
    wget
  ]) ++ [
    cgos
    cgos-util
    cgos-cgctl
  ];

  systemd.services = {
    congatec-exporter = {
      description = "Prometheus exporter for Congatec hardware monitor";
      wantedBy = [ "multi-user.target" ];
      after = [ "network.target" ];
      serviceConfig = {
        Type = "simple";
        Restart = "always";
        ExecStart = "${cgos-cgexporter}/bin/cgexporter";

        # Hardening
        DeviceAllow = "/dev/cgos rw";
        CapabilityBoundingSet = "CAP_SETUID CAP_SETGID";
        ProtectSystem = "strict";
        ProtectHome = true;
        MemoryDenyWriteExecute = true;
        PrivateTmp = true;
        ProtectControlGroups = true;
        ProtectKernelModules = true;
        ProtectKernelTunables = true;
        RestrictAddressFamilies = "AF_INET";
        RestrictNamespaces = true;
        RestrictRealtime = true;
        RestrictSUIDSGID = true;
        LockPersonality = true;
      };
    };
  };

  services = {
    udev.extraRules = ''
      KERNEL=="cgos", OWNER="root", GROUP="wheel", MODE="0660"
    '';

    # Enable the OpenSSH daemon.
    openssh = {
      enable = true;
      settings.PermitRootLogin = "yes";
    };
  };

  networking.firewall.enable = false;

  # Copy the NixOS configuration file and link it from the resulting system
  # (/run/current-system/configuration.nix). This is useful in case you
  # accidentally delete configuration.nix.
  # system.copySystemConfiguration = true;

  # This option defines the first version of NixOS you have installed on this particular machine,
  # and is used to maintain compatibility with application data (e.g. databases) created on older NixOS versions.
  #
  # Most users should NEVER change this value after the initial install, for any reason,
  # even if you've upgraded your system to a new NixOS release.
  #
  # This value does NOT affect the Nixpkgs version your packages and OS are pulled from,
  # so changing it will NOT upgrade your system.
  #
  # This value being lower than the current NixOS release does NOT mean your system is
  # out of date, out of support, or vulnerable.
  #
  # Do NOT change this value unless you have manually inspected all the changes it would make to your configuration,
  # and migrated your data accordingly.
  #
  # For more information, see `man configuration.nix` or https://nixos.org/manual/nixos/stable/options#opt-system.stateVersion .
  system.stateVersion = "23.11"; # Did you read the comment?
}
