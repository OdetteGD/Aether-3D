package com.aether3d;

import android.app.Activity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public final class AetherActivity extends Activity {
    private AetherSurfaceView view;

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        view = new AetherSurfaceView(this);
        setContentView(view);
    }

    @Override public void onDestroy() {
        if (view != null) view.shutdown();
        super.onDestroy();
    }

    private static final class AetherSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
        private boolean ready;

        AetherSurfaceView(Activity activity) {
            super(activity);
            getHolder().addCallback(this);
        }

        void shutdown() {
            if (ready) {
                AetherNative.shutdown();
                ready = false;
            }
        }

        @Override public void surfaceCreated(SurfaceHolder holder) { }

        @Override public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            if (width <= 0 || height <= 0) return;
            if (!ready) ready = AetherNative.initialize(holder.getSurface(), width, height);
            else ready = AetherNative.resize(width, height);
        }

        @Override public void surfaceDestroyed(SurfaceHolder holder) { shutdown(); }
    }
}
