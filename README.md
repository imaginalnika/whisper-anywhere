<div align="center">
  <h1>xhisper</h1>
  <i>/ˈzɪspər/</i>
  <br>
  <b>Dictate anywhere in Linux. Transcription at your cursor.</b>
  <br><br>
</div>

## Features

- **Cursor Integration**: Transcription appears directly where you're typing
- **Smart Model Selection**: Automatic model switching (turbo for short, large for long recordings)
- **Minimal Interface**: Simple status messages at cursor: `(recording...)` → `(transcribing...)`
- **Fast**: Groq Whisper API for near-instant transcription
- **Lightweight**: Single bash script, ~90 lines

---

## How It Works

1. Run the script to start recording: `(recording...)` appears at cursor
2. Run again to stop and transcribe: switches to `(transcribing...)`
3. Transcription replaces the status message at cursor

The script uses `whisper-large-v3-turbo` for recordings under 1000s and `whisper-large-v3` for longer recordings.

---

## Dependencies

- `pipewire` and `pipewire-utils` (audio recording)
- `wl-clipboard` (Wayland) or `xclip` (X11) (clipboard access)
- `jq`, `curl`, `ffmpeg` (processing)
- `gcc` (to build xhisper)
- Groq API key (free tier available at [console.groq.com](https://console.groq.com))

<details>
<summary>Fedora / RHEL / AlmaLinux / Rocky</summary>
<pre><code>sudo dnf install -y pipewire pipewire-utils wl-clipboard jq curl ffmpeg gcc</code></pre>
</details>

<details>
<summary>Arch Linux / Manjaro</summary>
<pre><code>sudo pacman -S pipewire wl-clipboard jq curl ffmpeg gcc</code></pre>
</details>

<details>
<summary>Debian / Ubuntu / Linux Mint</summary>
<pre><code>sudo apt update
sudo apt install pipewire wl-clipboard jq curl ffmpeg gcc</code></pre>
</details>

<details>
<summary>Void Linux</summary>
<pre><code>sudo xbps-install -S
sudo xbps-install pipewire wl-clipboard jq curl ffmpeg gcc</code></pre>
</details>

<details>
<summary>OpenSUSE (Leap / Tumbleweed)</summary>
<pre><code>sudo zypper refresh
sudo zypper install pipewire wl-clipboard jq curl ffmpeg gcc</code></pre>
</details>

**Note for X11 users:** Replace `wl-clipboard` with `xclip` in the commands above.

### User Permissions

Your user must be in the `input` group to access `/dev/uinput`:

```sh
sudo usermod -aG input $USER
```

Then **log out and log back in** for the group change to take effect.

---

## Setup

1. Get a Groq API key from [console.groq.com](https://console.groq.com)

2. Add to `~/.env`:
```sh
export GROQ_API_KEY="your_key_here"
```

3. Clone the repository:
```sh
git clone https://github.com/imaginalnika/xhisper.git
cd xhisper
```

4. Build and install:
```sh
make
sudo make install
```

5. Bind to a key (example with keyd):
```ini
[main]
capslock = layer(dictate)

[dictate:C]
d = macro(xhisper)
```

---

## Configuration

Edit variables at the top of `xhisper`:

| Variable                     | Default | Description                                      |
|------------------------------|---------|--------------------------------------------------|
| `LONG_RECORDING_THRESHOLD`   | `1000`  | Seconds threshold for large model (in seconds)   |
| `TRANSCRIPTION_PROMPT`       | Custom  | Context words for better Whisper accuracy        |

### Keyboard Layout Switching

If you use a non-QWERTY layout (Dvorak, Colemak, etc.), xhisper needs to switch to QWERTY temporarily for proper key simulation. Use a wrap key option:

```sh
xhisper --rightalt     # Press Right Alt before/after typing
xhisper --leftalt      # Press Left Alt before/after typing
xhisper --super        # Press Super (Windows key) before/after typing
```

**Available wrap keys:** `--leftalt`, `--rightalt`, `--leftctrl`, `--rightctrl`, `--leftshift`, `--rightshift`, `--super`

The wrap key is pressed before and after each paste operation, allowing you to configure your system to toggle keyboard layouts on that keypress.

---

## Known Limitations

**Terminal Applications**: The clipboard paste functionality uses Ctrl+V, which doesn't work in terminal emulators (they require Ctrl+Shift+V).

**Workaround**: Remap Ctrl+V to paste in your terminal emulator's settings.

**Note**: This limitation only affects international/Unicode characters. ASCII characters (a-z, A-Z, 0-9, punctuation) are typed directly and work in all applications including terminals.

---

## Usage

Simply run `xhisper` twice:
- **First run**: Starts recording
- **Second run**: Stops recording and transcribes

The transcription will be typed at your cursor position.

For non-QWERTY layouts, use a wrap key:
```sh
xhisper --rightalt
```

The daemon (`xhispertoold`) auto-starts when needed.

---

<p align="center">
  <em>Simple dictation for Linux</em>
</p>
