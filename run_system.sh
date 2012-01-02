#!/usr/bin/env bash
TERM_CMD='gnome-terminal'
TERM_CMD="$TERM_CMD --tab --title=\"BANK\"  --command=\"./build/bin/bank  49152\""
TERM_CMD="$TERM_CMD --tab --title=\"PROXY\" --command=\"./build/bin/proxy 49153 49152\""
TERM_CMD="$TERM_CMD --tab --title=\"ATM\"   --command=\"./build/bin/atm   49153\""
bash -c "$TERM_CMD"
