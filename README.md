# asi-echoclip 🎙️📋

**asi-echoclip** to lekka, natywna aplikacja C++ (GTK3 / Wayland / Hyprland) służąca jako asystent mowy i schowka. Umożliwia błyskawiczne dyktowanie głosem (STT), transkrypcję przez lokalny serwer Whisper.cpp, automatyczne kopiowanie tekstu do schowka (`wl-copy`), podgląd widma audio w czasie rzeczywistym (FFT) oraz trwałą historię w SQLite.

Projekt został zaprojektowany z myślą o dalszym rozwoju w kierunku pełnego dwukierunkowego asystenta mowy (w planach: integracja lokalnego silnika TTS do czytania zaznaczonego tekstu na głos).

---

## 🚀 Główne funkcje

- **Globalny skrót klawiszowy (`Super + B` / `$mainMod + B`)**:
  - Błyskawicznie wywołuje wycentrowane, pływające okno asystenta na pulpicie Wayland (Hyprland).
- **Transkrypcja na żywo (STT)**:
  - Sterowanie nagrywaniem jednym klawiszem (`Spacja`) lub dedykowanym przyciskiem.
  - **Wizualizator widma Fouriera (FFT)** w czasie rzeczywistym (40 pasm z gradientem i markerami Peak Caps) — pewność, że mikrofon poprawnie rejestruje Twój głos.
  - Stoper nagrywania na żywo.
  - Automatyczne wysyłanie audio WAV do lokalnego serwera Whisper (`/inference`).
  - Natychmiastowe kopiowanie transkrypcji do schowka systemowego (`wl-copy` oraz schowek GTK).
  - Opcja odsłuchania zarejestrowanego nagrania przed lub po skopiowaniu.
- **Trwała historia (SQLite)**:
  - Automatyczny zapis każdej transkrypcji w bazie SQLite (`~/.local/share/asi-echoclip/history.db`).
  - Wbudowana wyszukiwarka historii w czasie rzeczywistym.
  - Podwójne kliknięcie na dowolny wpis na liście natychmiast kopiuje go do schowka.
- **Planowany rozwój (Roadmap)**:
  - Obsługa syntezy mowy (TTS) – odczytywanie zawartości schowka na głos.
- **Środowisko NixOS**:
  - Gotowe środowisko deweloperskie i biblioteki w `shell.nix`.

---

## 🛠️ Wymagania

- **Wayland** z obsługą `wl-clipboard` (`wl-copy`).
- Dźwięk przez **PipeWire** (`pw-record`).
- Działający lokalny serwer **Whisper.cpp** (domyślnie nasłuchujący na porcie `8999` pod adresem `http://127.0.0.1:8999`).

---

## 📦 Budowanie i uruchamianie

### NixOS / nix-shell (zalecane)

```bash
git clone https://github.com/Sticzu/asi-echoclip.git
cd asi-echoclip

# Zbudowanie programu:
nix-shell --run "make"

# Uruchomienie:
./run.sh
```

### Tradycyjne środowisko Linux

Wymagane pakiety deweloperskie: `g++`, `make`, `pkg-config`, `gtkmm-3.0`, `libcurl`, `sqlite3`, `nlohmann-json`.

```bash
make
./asi-echoclip
```

---

## ⌨️ Skróty klawiszowe w oknie

| Skrót | Akcja |
|---|---|
| `Spacja` | Rozpocznij / zatrzymaj nagrywanie i transkrybuj |
| `Escape` | Zamknij okno asystenta |
| `Ctrl + C` | Kopiuj zaznaczony tekst |

---

## 🖥️ Konfiguracja Hyprland

W pliku `~/.config/hypr/hyprland.conf`:

```ini
# Skrót wywołania
bind = $mainMod, B, exec, /ścieżka/do/asi-echoclip/run.sh

# Reguły pływającego okna
windowrule = match:class ^(asi-echoclip)$, float 1
windowrule = match:class ^(asi-echoclip)$, center 1
windowrule = match:class ^(asi-echoclip)$, size 720 560
```
