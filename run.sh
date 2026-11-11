#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$DIR/asi-echoclip"
[ ! -f "$BIN" ] && [ -f "$DIR/whisper-clipboard" ] && BIN="$DIR/whisper-clipboard"

# Jeśli binarka jeszcze nie istnieje, zbuduj ją
if [ ! -f "$BIN" ]; then
    echo "Budowanie asi-echoclip..."
    if command -v nix-shell >/dev/null 2>&1; then
        nix-shell "$DIR/shell.nix" --run "cd '$DIR' && make"
    elif command -v make >/dev/null 2>&1 && command -v g++ >/dev/null 2>&1; then
        (cd "$DIR" && make)
    else
        echo "Błąd: Brak narzędzi do kompilacji (nix-shell lub g++ i make)."
        echo "W systemach Debian / Ubuntu / Linux Mint zainstaluj zależności poleceniem:"
        echo "  sudo apt install -y g++ make pkg-config libgtkmm-3.0-dev libcurl4-openssl-dev libsqlite3-dev nlohmann-json3-dev wl-clipboard xclip"
        exit 1
    fi
    BIN="$DIR/asi-echoclip"
fi

# Domyślny adres serwera Whisper (jeśli nie ustawiono z zewnątrz)
if [ -z "$WHISPER_SERVER_URL" ]; then
    if curl -s -m 1 http://127.0.0.1:8999/ >/dev/null 2>&1; then
        export WHISPER_SERVER_URL="http://127.0.0.1:8999"
    elif curl -s -m 1 http://host.docker.internal:8999/ >/dev/null 2>&1; then
        export WHISPER_SERVER_URL="http://host.docker.internal:8999"
    elif curl -s -m 1 http://sticzupc:8999/ >/dev/null 2>&1; then
        export WHISPER_SERVER_URL="http://sticzupc:8999"
    elif curl -s -m 1 http://100.72.137.46:8999/ >/dev/null 2>&1; then
        export WHISPER_SERVER_URL="http://100.72.137.46:8999"
    fi
fi

# Dostosowanie backendu GDK dla XFCE4 / X11 oraz Wayland
if [ -z "$GDK_BACKEND" ]; then
    if [ "$XDG_CURRENT_DESKTOP" = "XFCE" ] || [ -z "$WAYLAND_DISPLAY" ]; then
        export GDK_BACKEND=x11
    else
        export GDK_BACKEND=wayland,x11
    fi
fi

exec "$BIN" "$@"
