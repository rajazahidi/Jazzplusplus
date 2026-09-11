#!/bin/bash
# =============================================================================
# Jazz++ Smart Launcher with ALSA MIDI & Synthesizer Auto-Detection
# =============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Determine Jazz++ binary location
find_jazz_binary() {
    if [ -n "$APPDIR" ] && [ -x "$APPDIR/usr/bin/jazz" ]; then
        echo "$APPDIR/usr/bin/jazz"
    elif [ -x "$SCRIPT_DIR/../build/jazz" ]; then
        echo "$SCRIPT_DIR/../build/jazz"
    elif [ -x "$SCRIPT_DIR/jazz" ]; then
        echo "$SCRIPT_DIR/jazz"
    elif command -v jazz >/dev/null 2>&1; then
        command -v jazz
    elif [ -x "/usr/local/bin/jazz" ]; then
        echo "/usr/local/bin/jazz"
    elif [ -x "/usr/bin/jazz" ]; then
        echo "/usr/bin/jazz"
    else
        echo ""
    fi
}

# Check if an ALSA MIDI synth is already active
is_synth_active() {
    if command -v aconnect >/dev/null 2>&1; then
        if aconnect -o 2>/dev/null | grep -Eqi "FLUID Synth|TiMidity|Synth|QSynth"; then
            return 0
        fi
    fi
    return 1
}

# Find an available SoundFont on the system
find_soundfont() {
    local sf_paths=(
        "/usr/share/sounds/sf2/default-GM.sf2"
        "/usr/share/sounds/sf2/FluidR3_GM.sf2"
        "/usr/share/sounds/sf3/default-GM.sf3"
        "/usr/share/soundfonts/default.sf2"
        "/usr/share/sounds/sf2/TimGM6mb.sf2"
        "/usr/share/sounds/sf2/GeneralUser_GS.sf2"
        "$HOME/.sounds/default.sf2"
    )
    for sf in "${sf_paths[@]}"; do
        if [ -f "$sf" ]; then
            echo "$sf"
            return 0
        fi
    done
    return 1
}

# Diagnostic check mode
if [ "$1" = "--check" ]; then
    echo "=== Jazz++ Environment Check ==="
    BIN="$(find_jazz_binary)"
    if [ -n "$BIN" ]; then
        echo "[OK] Jazz++ binary: $BIN"
    else
        echo "[WARN] Jazz++ binary not found in standard locations"
    fi

    if is_synth_active; then
        echo "[OK] ALSA Synthesizer: Active"
        aconnect -o 2>/dev/null | grep -Ei "client.*:" | grep -v "Midi Through" || true
    else
        echo "[INFO] No active ALSA synthesizer detected."
        SF="$(find_soundfont || true)"
        if [ -n "$SF" ]; then
            echo "[OK] Found SoundFont: $SF"
        else
            echo "[WARN] No standard SoundFont found. (Install soundfont-fluid-gm or similar)"
        fi
        if command -v fluidsynth >/dev/null 2>&1; then
            echo "[OK] FluidSynth utility is available."
        else
            echo "[INFO] FluidSynth not installed. (Optional for auto-synth)"
        fi
    fi
    exit 0
fi

JAZZ_BIN="$(find_jazz_binary)"
if [ -z "$JAZZ_BIN" ]; then
    echo "Error: Could not locate jazz executable." >&2
    exit 1
fi

STARTED_SYNTH=0
FS_PID=""

cleanup() {
    if [ "$STARTED_SYNTH" -eq 1 ] && [ -n "$FS_PID" ]; then
        echo "Stopping background FluidSynth (PID $FS_PID)..."
        kill "$FS_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

# If no synth is active, try starting fluidsynth in background
if ! is_synth_active; then
    SF="$(find_soundfont || true)"
    if [ -n "$SF" ] && command -v fluidsynth >/dev/null 2>&1; then
        echo "Auto-starting background FluidSynth with $SF..."
        # Try pipewire first, then pulseaudio, then alsa default
        fluidsynth -a pipewire -m alsa_seq -s -i -j "$SF" >/dev/null 2>&1 &
        FS_PID=$!
        sleep 0.3
        if ! kill -0 "$FS_PID" 2>/dev/null; then
            fluidsynth -a pulseaudio -m alsa_seq -s -i -j "$SF" >/dev/null 2>&1 &
            FS_PID=$!
            sleep 0.3
            if ! kill -0 "$FS_PID" 2>/dev/null; then
                fluidsynth -a alsa -o audio.alsa.device=default -m alsa_seq -s -i -j "$SF" >/dev/null 2>&1 &
                FS_PID=$!
            fi
        fi
        if kill -0 "$FS_PID" 2>/dev/null; then
            STARTED_SYNTH=1
            echo "[OK] Background FluidSynth started (PID $FS_PID)"
        fi
    fi
fi

# Run Jazz++
if [ "$STARTED_SYNTH" -eq 1 ]; then
    "$JAZZ_BIN" "$@"
    EXIT_CODE=$?
    cleanup
    exit $EXIT_CODE
else
    exec "$JAZZ_BIN" "$@"
fi
