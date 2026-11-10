{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    gnumake
    pkg-config
    gtkmm3
    gtk3
    glib
    curl
    sqlite
    nlohmann_json
    pipewire
    wireplumber
    wl-clipboard
    libnotify
  ];

  shellHook = ''
    export LC_ALL=C.UTF-8
  '';
}
