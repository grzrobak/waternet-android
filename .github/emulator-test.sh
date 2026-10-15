#!/usr/bin/env bash
# Runs inside the Android emulator job: install, start, check the game keeps
# running, and save the log + screenshots.
set -x
mkdir -p test-output
adb install -r Waternet.apk
adb logcat -c
adb shell am start -W -n org.waternetport.android/.WaternetActivity
for i in 1 2 3; do
  sleep 5
  adb exec-out screencap -p > test-output/screen$i.png
done
# tap the middle of the screen (skips the intro), then look again
read W H < <(adb shell wm size | sed 's/.*: \([0-9]*\)x\([0-9]*\).*/\1 \2/')
adb shell input tap $((W / 2)) $((H / 2))
sleep 4
adb exec-out screencap -p > test-output/screen4.png
adb logcat -d > test-output/logcat.txt
set +x

GAME_LOG=$(grep -E "Waternet|SDL/APP|Finished main function|FATAL|AndroidRuntime: (FATAL|Process)|F DEBUG" test-output/logcat.txt | grep -E "org.waternetport|Waternet|SDL|DEBUG" | tail -40)
echo "$GAME_LOG"
# show the log as an annotation on the run (readable through the GitHub API)
echo "::notice title=Waternet emulator log::$(echo "$GAME_LOG" | sed 's/%/%25/g' | awk '{printf "%s%%0A", $0}')"

if grep -q "Finished main function" test-output/logcat.txt || ! adb shell pidof org.waternetport.android >/dev/null; then
  echo "::error title=Emulator test::The game stopped during the test"
  exit 1
fi
echo "::notice title=Emulator test::The game is running"
