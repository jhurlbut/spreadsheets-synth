#!/bin/bash

# This script will push the Spreadsheets Synth to GitHub
# Make sure you're logged in to GitHub as spreadsheetsmusic@gmail.com first

echo "Creating GitHub repository..."
gh repo create spreadsheets-synth --public \
  --description "A cultural product of the band Spreadsheets" \
  --homepage "https://www.instagram.com/spreadsheets_band/" \
  --source=. \
  --remote=origin \
  --push

echo "Repository created and pushed!"
echo "View at: https://github.com/spreadsheetsmusic/spreadsheets-synth"