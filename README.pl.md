# asi-echoclip 🎙️📋

**asi-echoclip** to lekka, natywna aplikacja C++ (GTK3 / Wayland / Hyprland) służąca jako asystent mowy i schowka. Umożliwia błyskawiczne dyktowanie głosem (STT), transkrypcję przez lokalny serwer Whisper.cpp, automatyczne kopiowanie tekstu do schowka (`wl-copy`), podgląd widma audio w czasie rzeczywistym (FFT) oraz trwałą historię w SQLite.

Projekt został zaprojektowany z myślą o dalszym rozwoju w kierunku pełnego dwukierunkowego asystenta mowy (w planach: integracja lokalnego silnika TTS do czytania zaznaczonego tekstu na głos).

[English version (README.md)](README.md)

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

- **Środowisko graficzne**:
  - **Wayland** (Hyprland, Sway itp.) z `wl-clipboard` (`wl-copy`) LUB
  - **X11 / XFCE4** z `xclip` lub `xsel` (oraz schowek GTK `clip->store()`).
- **System dźwięku**:
  - **PipeWire** (`pw-record` / `pw-play`), **PulseAudio** (`parec` / `paplay`) lub **ALSA** (`arecord` / `aplay`).
- Działający lokalny serwer **Whisper.cpp** (domyślnie nasłuchujący na porcie `8999` pod adresem `http://127.0.0.1:8999` lub `http://host.docker.internal:8999`).

---

## 📦 Budowanie i uruchamianie

### 1. Debian / Ubuntu / Linux Mint / XFCE4

Automatyczny instalator zależności i kompilacja:
```bash
./debian-install.sh
```

Ręczna instalacja pakietów:
```bash
sudo apt update
sudo apt install -y g++ make pkg-config libgtkmm-3.0-dev libcurl4-openssl-dev libsqlite3-dev nlohmann-json3-dev xclip wl-clipboard pipewire-bin alsa-utils pulseaudio-utils

make -j$(nproc)
sudo make install
```

Po instalacji program jest dostępny w menu aplikacji (w tym w Whisker Menu w XFCE) oraz z poziomu terminala jako `asi-echoclip`.

### 2. NixOS / nix-shell

```bash
git clone https://github.com/Sticzu/asi-echoclip.git
cd asi-echoclip

# Zbudowanie programu:
nix-shell --run "make"

# Uruchomienie:
./run.sh
```

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

---

## 🖱️ Konfiguracja XFCE4

### Automatyczna konfiguracja skrótu `Super + B`:
```bash
./setup-xfce-shortcut.sh
```

### Ręczna konfiguracja w XFCE:
1. Otwórz **Menu** $\rightarrow$ **Ustawienia** $\rightarrow$ **Klawiatura** $\rightarrow$ zakładka **Skróty klawiszowe programów**.
2. Kliknij **Dodaj**.
3. Jako polecenie wpisz: `asi-echoclip` (lub pełną ścieżkę do `run.sh`).
4. Wciśnij żądaną kombinację klawiszy (np. `Super + B`).

Okno w XFCE automatycznie otwiera się wycentrowane na ekranie i posiada zarejestrowaną klasę `asi-echoclip`.
