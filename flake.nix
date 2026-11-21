{
  description = "MicroSIP Native Linux/NixOS Port (Qt 6 + PJSIP)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        packages = {
          microsip-qt = pkgs.stdenv.mkDerivation {
            pname = "microsip-qt";
            version = "3.23.5-linux";

            src = ./.;

            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              pkg-config
              qt6.wrapQtAppsHook
            ];

            buildInputs = with pkgs; [
              qt6.qtbase
              qt6.qtmultimedia
              pjsip
              sqlite
              openssl
              opus
              alsa-lib
              libpulseaudio
            ];

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
            ];

            meta = with pkgs.lib; {
              description = "Native Linux/NixOS Qt6 port of MicroSIP (PJSIP-based SIP softphone)";
              homepage = "https://www.microsip.org";
              license = licenses.gpl2Plus;
              platforms = platforms.linux;
              mainProgram = "microsip-qt";
            };
          };
          default = self.packages.${system}.microsip-qt;
        };

        apps.default = flake-utils.lib.mkApp {
          drv = self.packages.${system}.default;
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
          packages = with pkgs; [
            gdb
            clang-tools
          ];
        };
      }
    ) // {
      nixosModules.default = { config, lib, pkgs, ... }:
        let
          cfg = config.programs.microsip-qt;
        in
        {
          options.programs.microsip-qt = {
            enable = lib.mkEnableOption "MicroSIP native Qt6 SIP softphone";
            package = lib.mkOption {
              type = lib.types.package;
              default = self.packages.${pkgs.stdenv.hostPlatform.system}.microsip-qt;
              description = "The microsip-qt package to install.";
            };
          };

          config = lib.mkIf cfg.enable {
            environment.systemPackages = [ cfg.package ];
          };
        };

      homeManagerModules.default = { config, lib, pkgs, ... }:
        let
          cfg = config.programs.microsip-qt;
        in
        {
          options.programs.microsip-qt = {
            enable = lib.mkEnableOption "MicroSIP native Qt6 SIP softphone";
            autostart = lib.mkEnableOption "Start MicroSIP minimized on graphical session login";
            package = lib.mkOption {
              type = lib.types.package;
              default = self.packages.${pkgs.stdenv.hostPlatform.system}.microsip-qt;
              description = "The microsip-qt package to install.";
            };
          };

          config = lib.mkIf cfg.enable {
            home.packages = [ cfg.package ];
            systemd.user.services.microsip-qt = lib.mkIf cfg.autostart {
              Unit = {
                Description = "MicroSIP Native Qt6 SIP Softphone";
                After = [ "graphical-session.target" "pipewire.service" ];
                PartOf = [ "graphical-session.target" ];
              };
              Service = {
                ExecStart = "${cfg.package}/bin/microsip-qt --minimized";
                Restart = "on-failure";
                RestartSec = 5;
              };
              Install.WantedBy = [ "graphical-session.target" ];
            };
          };
        };
    };
}
