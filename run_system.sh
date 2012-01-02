#!/usr/bin/env bash
BASE_DIR=$(dirname $0)
TERM_CMD='gnome-terminal'
TERM_CMD="${TERM_CMD} --tab --title=\"BANK\"  --command=\"${BASE_DIR}/build/bin/bank  49152\""
TERM_CMD="${TERM_CMD} --tab --title=\"PROXY\" --command=\"${BASE_DIR}/build/bin/proxy 49153 49152\""
TERM_CMD="${TERM_CMD} --tab --title=\"ATM\"   --command=\"${BASE_DIR}/build/bin/atm   49153\""
bash -c "${TERM_CMD}"
