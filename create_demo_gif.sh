#!/bin/bash

# SPREADSHEETS SYNTH DEMO CAPTURE PROTOCOL
# Transmission frequency: 303.909.808

echo "================================================"
echo "SPREADSHEETS SYNTH DEMO CAPTURE - PROTOCOL 313"
echo "================================================"
echo ""
echo "MANUAL RECORDING INSTRUCTIONS:"
echo ""
echo "1. The Spreadsheets Synth app should now be open"
echo "2. Press Cmd+Shift+5 to open macOS screen recording"
echo "3. Select 'Record Selected Portion' and frame the plugin window"
echo "4. Click 'Record' and demonstrate:"
echo "   - Click PLAY button to start sequencer"
echo "   - Click some step buttons to create a pattern"
echo "   - Move the XY pad for comb filter modulation"
echo "   - Click the [RND] button to randomize"
echo "   - Adjust some knobs (cutoff, resonance, etc)"
echo "   - Show the SPREADSHEETS letters lighting up"
echo "5. Stop recording after 10-15 seconds"
echo "6. Save the video as 'demo_recording.mov' in this directory"
echo ""
echo "Press Enter when you've saved the recording..."
read

# Check if recording exists
if [ ! -f "demo_recording.mov" ]; then
    echo "ERROR: demo_recording.mov not found!"
    echo "Please save the recording in: $(pwd)"
    exit 1
fi

echo ""
echo "CONVERTING TO GIF [DETROIT COMPRESSION ALGORITHM]..."
echo ""

# Generate palette for better GIF quality (303 optimization)
ffmpeg -i demo_recording.mov -vf "fps=15,scale=800:-1:flags=lanczos,palettegen=stats_mode=diff" -y palette_313.png

# Create the GIF with optimized settings (Underground Resistance protocol)
ffmpeg -i demo_recording.mov -i palette_313.png -filter_complex \
"fps=15,scale=800:-1:flags=lanczos[x];[x][1:v]paletteuse=dither=bayer:bayer_scale=5:diff_mode=rectangle" \
-y spreadsheets_synth_demo.gif

# Clean up temporary files
rm palette_313.png

# Calculate final transmission size
SIZE=$(du -h spreadsheets_synth_demo.gif | cut -f1)

echo ""
echo "================================================"
echo "TRANSMISSION COMPLETE"
echo "================================================"
echo "Output: spreadsheets_synth_demo.gif"
echo "Size: $SIZE"
echo "Frequency: 15 fps"
echo "Resolution: 800px wide"
echo "Codec: Detroit GIF Protocol"
echo ""
echo "The GIF can now be added to the README and GitHub release."
echo ""
echo "EOF [End of Frequency]"