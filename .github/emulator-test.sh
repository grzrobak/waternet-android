#!/usr/bin/env bash
# Runs inside the Android emulator job: install, start, collect log + screenshots.
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
adb shell input tap 540 1200
sleep 3
adb exec-out screencap -p > test-output/screen4.png
if adb shell pidof org.waternetport.android; then echo "RESULT: app is running"; else echo "RESULT: app is NOT running"; fi
adb logcat -d > test-output/logcat.txt
# show the interesting part in the job log
grep -E "Waternet|SDL|AndroidRuntime|DEBUG|libc|FATAL|crash|waternetport" test-output/logcat.txt | grep -v "^--------" | tail -150
