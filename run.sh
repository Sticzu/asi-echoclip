#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$DIR/asi-echoclip"
[ ! -f "$BIN" ] && [ -f "$DIR/whisper-clipboard" ] && BIN="$DIR/whisper-clipboard"

# Jeśli binarka jeszcze nie istnieje, zbuduj ją
if [ ! -f "$BIN" ]; then
    echo "Budowanie asi-echoclip..."
    nix-shell "$DIR/shell.nix" --run "cd '$DIR' && make"
    BIN="$DIR/asi-echoclip"
fi

# Domyślny adres serwera Whisper (jeśli nie ustawiono z zewnątrz)
# Na sticzupc łączy się lokalnie, na innych maszynach przez Tailscale
if [ -z "$WHISPER_SERVER_URL" ]; then
    if curl -s -m 1 http://127.0.0.1:8999/ >/dev/null 2>&1; then
        export WHISPER_SERVER_URL="http://127.0.0.1:8999"
    elif curl -s -m 1 http://sticzupc:8999/ >/dev/null 2>&1; then
        export WHISPER_SERVER_URL="http://sticzupc:8999"
    elif curl -s -m 1 http://100.72.137.46:8999/ >/dev/null 2>&1; then
        export WHISPER_SERVER_URL="http://100.72.137.46:8999"
    fi
fi

export GDK_BACKEND=wayland,x11
exec "$BIN" "$@"
