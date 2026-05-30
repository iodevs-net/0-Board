#!/bin/bash
# lock-monitor.sh - Monitoriza transiciones de bloqueo de pantalla en TDE
export DISPLAY=:0

# Determinar el estado inicial del bloqueo para evitar condiciones de carrera en el inicio
if pgrep -x kdesktop_lock >/dev/null; then
    LOCKED=1
else
    LOCKED=0
fi

while true; do
    if pgrep -x kdesktop_lock >/dev/null; then
        if [ $LOCKED -eq 0 ]; then
            pkill -x 0-board 2>/dev/null
            sleep 0.2
            /home/leonardo/.local/bin/0-board &
            LOCKED=1
        fi
    else
        if [ $LOCKED -eq 1 ]; then
            pkill -x 0-board 2>/dev/null
            sleep 0.2
            /home/leonardo/.local/bin/0-board &
            LOCKED=0
        fi
    fi
    sleep 2
done
