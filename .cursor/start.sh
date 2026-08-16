#!/usr/bin/env bash
#
# Cloud Agent start step for the All-Projects repository.
#
# The DINO_GAME LVGL + SDL2 simulator needs an X display, but the Cloud Agent
# VM is headless. This starts a virtual framebuffer (Xvfb) on display :99 so the
# simulator can render and be screenshotted. Run the simulator with:
#
#     DISPLAY=:99 ./DINO_GAME/build/dino_game
#
# The step is idempotent: it does nothing if display :99 already answers.
set -euo pipefail

DISPLAY_NUM=":99"

if DISPLAY="${DISPLAY_NUM}" xdpyinfo >/dev/null 2>&1; then
    echo "start.sh: Xvfb already running on ${DISPLAY_NUM}"
    exit 0
fi

Xvfb "${DISPLAY_NUM}" -screen 0 640x480x24 >/tmp/xvfb.log 2>&1 &

# Wait briefly for the display to come up so downstream tooling can rely on it.
for _ in $(seq 1 20); do
    if DISPLAY="${DISPLAY_NUM}" xdpyinfo >/dev/null 2>&1; then
        echo "start.sh: Xvfb ready on ${DISPLAY_NUM}"
        exit 0
    fi
    sleep 0.25
done

echo "start.sh: ERROR — Xvfb did not become ready on ${DISPLAY_NUM}" >&2
cat /tmp/xvfb.log >&2 || true
exit 1
