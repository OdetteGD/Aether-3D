package com.aether3d;

import android.app.Activity;
import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public final class AetherActivity extends Activity implements SurfaceHolder.Callback {
    private AetherSurfaceView view;

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        view = new AetherSurfaceView();
        setContentView(view);
    }

    @Override public void onDestroy() {
        if (view != null) view.shutdown();
        super.onDestroy();
    }

    private final class AetherSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
        private boolean ready;
        AetherSurfaceView() { super(AetherActivity.this); getHolder().addCallback(this); }
        void shutdown() { if (ready) { AetherNative.shutdown(); ready = false; } }
        @Override public void surfaceCreated(SurfaceHolder h) { }
        @Override public void surfaceChanged(SurfaceHolder h, int format, int w, int hgt) {
            if (!ready) ready = AetherNative.initialize(h.getSurface(), w, hgt);
            else AetherNative.resize(w, hgt);
        }
        @Override public void surfaceDestroyed(SurfaceHolder h) { shutdown(); }
    }

    private static final class AetherNative {
        static { System.loadLibrary("aether3d"); }
        static native boolean initialize(Surface surface, int width, int height);
        static native boolean resize(int width, int height);
        static native void shutdown();
    }
}
