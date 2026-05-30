#!/bin/bash
# touch-handler.sh - Touchscreen long-press to right-click daemon
export DISPLAY=:0

# Detect Touch ID of SYNA7508 touchscreen dynamically
TOUCH_ID=$(xinput list | grep "SYNA7508:00" | grep -v "Stylus" | grep -o -E "id=[0-9]+" | cut -d= -f2)

if [ -n "$TOUCH_ID" ]; then
    HOLD_MS=600
    pkill -f "xinput test.*SYNA7508" 2>/dev/null
    
    xinput test "$TOUCH_ID" | while read -r line; do
        case "$line" in
            "button press"*)
                DOWN=$(date +%s%N)
                MOVED=0
                ;;
            "motion"*)
                MOVED=1
                ;;
            "button release"*)
                if [ "$MOVED" = 0 ]; then
                    ELAPSED=$(( ($(date +%s%N) - DOWN) / 1000000 ))
                    if [ "$ELAPSED" -ge "$HOLD_MS" ]; then
                        xdotool click 3
                    fi
                fi
                ;;
        esac
    done
fi
