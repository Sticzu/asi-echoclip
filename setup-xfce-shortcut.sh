#!/usr/bin/env bash
# Skrypt konfiguracji globalnego skrótu klawiszowego Super+B w XFCE4 dla asi-echoclip
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$(command -v asi-echoclip || echo "$DIR/run.sh")"

if ! command -v xfconf-query >/dev/null 2>&1; then
    echo "Błąd: Brak narzędzia 'xfconf-query'. Uruchom ten skrypt w sesji środowiska XFCE."
    exit 1
fi

SHORTCUT_PROP="/commands/custom/<Super>b"

echo "Konfigurowanie skrótu Super+B -> $BIN w XFCE4..."
xfconf-query -c xfce4-keyboard-shortcuts -p "$SHORTCUT_PROP" -n -t string -s "$BIN" 2>/dev/null || \
xfconf-query -c xfce4-keyboard-shortcuts -p "$SHORTCUT_PROP" -s "$BIN"

echo "✅ Pomyślnie ustawiono skrót Super+B dla asi-echoclip w XFCE4!"
echo "Możesz go przetestować wciskając Super+B na klawiaturze."
