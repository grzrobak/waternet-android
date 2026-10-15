# Waternet for Android (port of the Dingoo version)

Waternet is a pipe puzzle by **Willems Davy – Willems Soft – http://www.willemssoft.be**.
This is an Android port of his original Dingoo (dingux) version, with the original
320×240 graphics, music, sounds, skins and all 180 levels plus the random
level generator, unchanged. The game code is the original `waternet.cpp`. It
was moved from SDL 1.2 to SDL2 through a small compatibility layer
(`src/port.cpp`), which also adds touch controls.

The original source code and its license are in `docs/`. Please respect the
author's conditions: the game must stay **free**, the splash screens must not be
changed, and any release must say the game was made by Willems Davy
(http://www.willemssoft.be). He also asked to be told about ports.

## Getting the APK on your phone (no software to install)

1. Create a free account on https://github.com and make a new repository.
2. Upload the contents of this folder to it (drag and drop on the repository
   page works, including the hidden `.github` folder).
3. Open the **Actions** tab. The "Build Android APK" job starts by itself and
   takes about 10 minutes. If it doesn't start, click it and press "Run workflow".
4. When it's done, open **Releases → latest** on your phone and download
   `Waternet.apk`. Open it and allow installing from your browser when Android asks.

New pushes rebuild the APK. Every build is signed with the same key
(`android/app/waternet.keystore`), so an update installs over the old version
and keeps your progress.

## Building with Android Studio instead

```
bash android/fetch-libs.sh        # downloads SDL2 + SDL_image/mixer/ttf (needs git)
```
Then open the `android` folder in Android Studio and press Run, or run
`./gradlew assembleRelease` inside `android/`. The APK is in
`android/app/build/outputs/apk/release/`. On Windows, run the script from Git Bash.

## Controls

| Dingoo | Phone |
|---|---|
| D-pad | on-screen D-pad (hold to repeat) |
| A | on-screen **A**, or **tap a pipe** to move the cursor there and rotate/slide it |
| SELECT (back) | on-screen **SELECT** or the Android back gesture/button |
| L / R (change skin) | on-screen **L** / **R** |
| X / Y (volume) | the phone's volume buttons |
| – | on-screen **MUSIC**: turns the music off/on (sound effects stay on); remembered |

Also:

* **B** rotates the other way in-game, like on the GP2X. The Dingoo version didn't use B.
* Title menu: tap Play / Credits / Quit.
* Game type and level type menus: tap the middle to choose, tap the left or right side (or swipe) to browse.
* Level selector: tap the board to play it. Tap near its left or right edge, or swipe, to browse levels.
* Any other screen: tap to continue.
* Sliding modes: tap an arrow to slide that row or column.
* Gamepads and keyboards work too: arrows, Enter/Z = A, X = B, Esc = SELECT, Q/E = L/R, M = music on/off.

Both landscape (controls left and right of the picture) and portrait
(controls underneath) work. Rotate the phone to switch.

## Desktop build (for testing)

With SDL2, SDL2_image, SDL2_mixer and SDL2_ttf installed:
```
cmake -B build && cmake --build build && ./build/waternet
```

## What changed from the original

* `src/port.h`, `src/port.cpp`: new. They provide SDL 1.2 functions the game uses
  (`SDL_SetVideoMode`, `SDL_Flip`, `SDL_DisplayFormat`, `SDL_SetAlpha`, plus `zoomSurface`
  and `boxRGBA` from SDL_gfx) on top of SDL2. They also handle sharp scaling, the touch
  controls, gamepads, and unpacking the data files on first start.
* `src/gp2x.h`: the Dingoo button codes are now internal codes.
* `src/waternet.cpp`: SDL 1.2 includes replaced, GP2X joystick setup removed, a larger
  audio buffer, a tap handler, and the B button in-game. All marked "android port".
  Progress is saved when the app goes to the background.
