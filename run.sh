#!/bin/bash
# run.sh - Inicia 0-Board con las variables correctas
export DISPLAY=:0
pkill -9 -x 0-board 2>/dev/null
sleep 0.1
exec /home/leonardo/.local/bin/0-board >/dev/null 2>&1
