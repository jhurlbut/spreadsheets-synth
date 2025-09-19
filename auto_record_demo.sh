#!/bin/bash

# AUTOMATIC DEMO CAPTURE - PROTOCOL 909

echo "================================================"
echo "AUTOMATED CAPTURE SEQUENCE INITIATED"
echo "================================================"
echo ""
echo "Position the Spreadsheets Synth window in the center of your screen."
echo "Make sure the plugin is visible and ready."
echo ""
echo "Press Enter to start 10-second recording in 3 seconds..."
read

echo "Recording will start in..."
for i in 3 2 1; do
    echo "$i..."
    sleep 1
done

echo "RECORDING [TRANSMITTING ON 313 FREQUENCY]..."

# Record for 10 seconds (using screencapture video mode)
# This captures the entire screen - you can crop later
screencapture -v -t 10 demo_recording.mov

echo ""
echo "Recording complete. Converting to GIF..."
echo ""

# Check if recording was created
if [ ! -f "demo_recording.mov" ]; then
    echo "Recording failed. Trying alternative method..."
    echo ""
    echo "Please use QuickTime Player:"
    echo "1. Open QuickTime Player"
    echo "2. File > New Screen Recording"
    echo "3. Click Record and select the plugin window"
    echo "4. Demonstrate the plugin for 10-15 seconds"
    echo "5. Stop and save as 'demo_recording.mov' here"
    exit 1
fi

# Convert to GIF with Detroit parameters
echo "Applying 303 compression algorithm..."

# Create optimized GIF
ffmpeg -i demo_recording.mov \
    -vf "fps=15,scale=800:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" \
    -loop 0 \
    spreadsheets_synth_demo.gif

echo ""
echo "================================================"
echo "TRANSMISSION COMPLETE"
echo "================================================"
echo "Output: spreadsheets_synth_demo.gif"
echo ""

# Display file info
ls -lh spreadsheets_synth_demo.gif