package com.aether3d;

import android.view.Surface;

final class AetherNative {
    static { System.loadLibrary("aether3d"); }
    private AetherNative() { }
    static native boolean initialize(Surface surface, int width, int height);
    static native boolean resize(int width, int height);
    static native void shutdown();
}
