package org.waternetport.android;

import org.libsdl.app.SDLActivity;

/** Waternet (Willems Davy, Willems Soft) - Android port of the Dingoo version. */
public class WaternetActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {
            "SDL2",
            "SDL2_image",
            "SDL2_mixer",
            "SDL2_ttf",
            "main"
        };
    }
}
