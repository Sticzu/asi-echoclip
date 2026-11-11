#!/usr/bin/env bash
# Skrypt automatycznej instalacji zależności i budowy asi-echoclip dla Debian / Ubuntu / Linux Mint
set -e

# Obsługa flag nieinteraktywnych (-y / --yes)
AUTO_YES=false
for arg in "$@"; do
    case "$arg" in
        -y|--yes) AUTO_YES=true ;;
    esac
done

if [ ! -t 0 ]; then
    AUTO_YES=true
fi

# Wykrywanie katalogu ze źródłami (obsługa pipe z curl: curl ... | bash)
DIR="$(pwd)"
CLEANUP_DIR=""
if [ ! -f "$DIR/Makefile" ] || [ ! -f "$DIR/src/main.cpp" ]; then
    echo "=== Pobieranie źródeł asi-echoclip z GitHub ==="
    TMP_DIR="/tmp/asi-echoclip-build-$$"
    mkdir -p "$TMP_DIR"
    CLEANUP_DIR="$TMP_DIR"
    curl -sSL https://github.com/Sticzu/asi-echoclip/archive/refs/heads/main.tar.gz | tar -xz -C "$TMP_DIR" --strip-components=1
    DIR="$TMP_DIR"
fi

cleanup() {
    if [ -n "$CLEANUP_DIR" ] && [ -d "$CLEANUP_DIR" ]; then
        rm -rf "$CLEANUP_DIR"
    fi
}
trap cleanup EXIT

echo "=== 1. Instalacja zależności dla Debian / Ubuntu / Linux Mint ==="
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
    alsa-utils \
    fonts-noto-color-emoji

echo "=== 2. Budowanie asi-echoclip ==="
(cd "$DIR" && make clean && make -j"$(nproc)")

echo "=== 3. Instalacja systemowa (/usr/local/bin) ==="
DO_INSTALL=false
if [ "$AUTO_YES" = true ]; then
    DO_INSTALL=true
else
    read -p "Czy chcesz zainstalować asi-echoclip w systemie (/usr/local/bin)? [T/n] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[TtYy]$ ]] || [[ -z $REPLY ]]; then
        DO_INSTALL=true
    fi
fi

if [ "$DO_INSTALL" = true ]; then
    $SUDO make -C "$DIR" install
    echo "✅ Zainstalowano pomyślnie w /usr/local/bin/asi-echoclip!"
    echo "Plik .desktop zarejestrowano w /usr/local/share/applications/."

    # Automatyczna konfiguracja skrótu w XFCE jeśli wykryto sesję
    if command -v xfconf-query >/dev/null 2>&1 && [ -n "$DISPLAY" ]; then
        xfconf-query -c xfce4-keyboard-shortcuts -p "/commands/custom/<Super>b" -s "/usr/local/bin/asi-echoclip" --create -t string 2>/dev/null && \
        echo "✅ Skrót Super+B został automatycznie skonfigurowany w XFCE4!" || true
    fi
fi

echo "=== Gotowe! Możesz uruchomić program poleceniem: asi-echoclip (lub ./run.sh) ==="
