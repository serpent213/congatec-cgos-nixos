# Heimchen Home Automation (Grabenweg)

## Deploy to bare metal

* Boot installer ISO

## Build

* `sudo nixos-rebuild switch --flake .#heimchen`
* `sudo nixos-rebuild switch --flake .#heimchen --target-host user@system`

## Debug

* `echo module wireguard +p > /sys/kernel/debug/dynamic_debug/control`

## Congatec BSP

* https://git.congatec.com/x86/meta-congatec-x86

## Further reading

* https://github.com/serokell/deploy-rs
* https://github.com/MatthewCroughan/nixinate
