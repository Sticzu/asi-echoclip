#!/usr/bin/env bash
# Skrypt automatycznej instalacji zależności i budowy asi-echoclip dla Debian / Ubuntu / Linux Mint
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== Instalacja zależności dla Debian / Ubuntu / Linux Mint ==="
if [ "$EUID" -ne 0 ]; then
    SUDO="sudo"
else
    SUDO=""
fi

$SUDO apt-get update
$SUDO apt-get install -y --no-install-recommends \
    g++ \
    make \
    pkg-config \
    libgtkmm-3.0-dev \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    nlohmann-json3-dev \
    xclip \
    wl-clipboard \
    pipewire-bin \
    pulseaudio-utils \
    alsa-utils

echo "=== Budowanie asi-echoclip ==="
(cd "$DIR" && make clean && make -j"$(nproc)")

echo "=== Instalacja systemowa (opcjonalnie) ==="
read -p "Czy chcesz zainstalować asi-echoclip w systemie (/usr/local/bin)? [T/n] " -n 1 -r
echo
if [[ $REPLY =~ ^[TtYy]$ ]] || [[ -z $REPLY ]]; then
    $SUDO make -C "$DIR" install
    echo "✅ Zainstalowano pomyślnie! Program jest dostępny w menu aplikacji i jako 'asi-echoclip'."
fi

echo "=== Gotowe! Możesz uruchomić program poleceniem: ./run.sh ==="
