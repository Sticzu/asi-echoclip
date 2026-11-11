# asi-echoclip 🎙️📋

[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Wayland%20%7C%20Hyprland-blue.svg)](https://hyprland.org)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17-orange.svg)]()
[![Toolkit](https://img.shields.io/badge/GUI-GTKmm%203.0-green.svg)](https://www.gtkmm.org)
[![License](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](LICENSE)

[English](README.md) | [Polski](README.pl.md)

**asi-echoclip** is a lightweight, native C++ desktop voice and clipboard assistant designed for Linux / Wayland (specifically tailored for Hyprland). It enables instant voice dictation (Speech-to-Text) powered by a local Whisper.cpp server, automatic clipboard synchronization (`wl-copy`), real-time Fast Fourier Transform (FFT) spectrum audio visualization, and persistent history tracking with SQLite.

The project is architected for two-way voice workflows, with upcoming Text-to-Speech (TTS) integration to read highlighted clipboard text aloud.

---

## 🚀 Key Features

- **Global Hotkey Launch (`Super + B` / `$mainMod + B`)**:
  - Pops up a centered, floating assistant overlay window instantly in Wayland/Hyprland.
- **Real-Time Speech-to-Text (STT)**:
  - Intuitive one-key control via `Spacebar` or the on-screen mic toggle.
  - **Live Fourier Spectrum Visualizer (FFT)**: 40 frequency bands with gradient coloring and Peak Cap falloff indicators, providing instant visual feedback that the microphone is capturing audio.
  - Live recording duration counter.
  - Automatic audio dispatch (WAV) to the local Whisper.cpp inference endpoint (`/inference`).
  - Seamless clipboard injection (`wl-copy` & GTK clipboard).
  - Quick audio playback button (`▶ Play Recording`) to verify captured audio before or after transcription.
- **Persistent SQLite History**:
  - Automatically records all transcriptions in a local SQLite database (`~/.local/share/asi-echoclip/history.db`).
  - Real-time search filter through past transcription records.
  - Double-clicking any past entry immediately copies it back to the system clipboard.
  - Full management: delete individual items or wipe the entire history.
- **Roadmap**:
  - Integrated local TTS (Text-to-Speech) generation: read selected text aloud on demand.
- **NixOS Ready**:
  - Complete, reproducible development and runtime environment defined in `shell.nix`.

---

## 🛠️ Prerequisites

- **Desktop Environments**:
  - **Wayland** (Hyprland, Sway, etc.) with `wl-clipboard` (`wl-copy`), OR
  - **X11 / XFCE4** with `xclip` or `xsel` (plus standard GTK clipboard storage).
- **Audio Subsystem**:
  - **PipeWire** (`pw-record` / `pw-play`), **PulseAudio** (`parec` / `paplay`), or **ALSA** (`arecord` / `aplay`).
- Running local **Whisper.cpp** HTTP server (default: `http://127.0.0.1:8999` or `http://host.docker.internal:8999`).

---

## 📦 Building & Running

### 1. Debian / Ubuntu / Linux Mint / XFCE4

Automated dependency installation and build:
```bash
./debian-install.sh
```

Or manual build:
```bash
sudo apt update
sudo apt install -y g++ make pkg-config libgtkmm-3.0-dev libcurl4-openssl-dev libsqlite3-dev nlohmann-json3-dev xclip wl-clipboard pipewire-bin alsa-utils pulseaudio-utils

make -j$(nproc)
sudo make install
```

### 2. NixOS / nix-shell

```bash
git clone https://github.com/Sticzu/asi-echoclip.git
cd asi-echoclip

# Build with nix-shell dependencies:
nix-shell --run "make"

# Launch:
./run.sh
```
---

## ⌨️ In-App Hotkeys

| Shortcut | Description |
|---|---|
| `Space` | Start / Stop recording & trigger transcription |
| `Escape` | Dismiss / close the assistant window |
| `Ctrl + C` | Copy highlighted text to clipboard |

---

## 🖥️ Hyprland Integration

Add the following configuration to `~/.config/hypr/hyprland.conf`:

```ini
# Global hotkey
bind = $mainMod, B, exec, /path/to/asi-echoclip/run.sh

# Floating window placement rules
windowrule = match:class ^(asi-echoclip)$, float 1
windowrule = match:class ^(asi-echoclip)$, center 1
windowrule = match:class ^(asi-echoclip)$, size 720 560
```

---

## 🖱️ XFCE4 Integration

### Automated hotkey configuration (`Super + B`):
```bash
./setup-xfce-shortcut.sh
```

### Manual configuration in XFCE:
1. Open **Menu** $\rightarrow$ **Settings** $\rightarrow$ **Keyboard** $\rightarrow$ **Application Shortcuts** tab.
2. Click **Add**.
3. Enter command: `asi-echoclip` (or full path to `run.sh`).
4. Press your desired shortcut (e.g. `Super + B`).

In XFCE the window is automatically centered and sized properly.
