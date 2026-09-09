#!/usr/bin/env bash
set -e

SF_DIR="/home/kingzahidi/jazz/soundfonts"
CONF_FILE="/home/kingzahidi/.config/fluidsynth"

CHOICE="${1:-musescore}"

OPTS="-a pipewire -m alsa_seq -r 48000 -o synth.sample-rate=48000 -o synth.cpu-cores=4 -o synth.polyphony=512 -o synth.gain=0.45 -o synth.reverb.active=yes -o synth.chorus.active=yes"

case "$CHOICE" in
  musescore|musescore_general|default)
    echo "Setting SoundFont: MuseScore General Full (Studio Acoustic Quality)..."
    SF="$SF_DIR/MuseScore_General_Full.sf3 $SF_DIR/FluidR3_GS.sf2"
    ;;
  generaluser|generaluser_gs)
    echo "Setting SoundFont: GeneralUser GS (Warm Acoustic Balance)..."
    SF="$SF_DIR/GeneralUser-GS.sf2"
    ;;
  fluidr3|fluid)
    echo "Setting SoundFont: FluidR3 GM+GS (Large Symphonic Library)..."
    SF="/usr/share/sounds/sf2/FluidR3_GM.sf2 $SF_DIR/FluidR3_GS.sf2"
    ;;
  *)
    echo "Usage: $0 [musescore | generaluser | fluidr3]"
    exit 1
    ;;
esac

cat << CONF_EOF > "$CONF_FILE"
SOUND_FONT="$SF"
OTHER_OPTS="$OPTS"
CONF_EOF

echo "Restarting FluidSynth with PipeWire..."
systemctl --user restart fluidsynth
echo "SoundFont updated successfully! Current status:"
systemctl --user status fluidsynth --no-pager | head -n 12
