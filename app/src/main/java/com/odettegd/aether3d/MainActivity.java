package com.odettegd.aether3d;

import android.app.Activity;
import android.os.Bundle;
import android.view.*;
import android.widget.*;
import android.graphics.Color;
import android.content.Context;

public final class MainActivity extends Activity {
    static { System.loadLibrary("aether3d"); }
    private AetherSurface surface;
    private TextView status;
    private native void nativeSetSurface(Surface s);
    private native void nativeResize(int w, int h);
    private native void nativeTouch(float x, float y, int action);

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        FrameLayout root = new FrameLayout(this);
        surface = new AetherSurface(this);
        root.addView(surface, new FrameLayout.LayoutParams(-1, -1));
        LinearLayout toolbar = new LinearLayout(this);
        toolbar.setOrientation(LinearLayout.HORIZONTAL);
        toolbar.setPadding(18,10,18,10);
        toolbar.setBackgroundColor(0xCC080B10);
        status = new TextView(this);
        status.setTextColor(Color.WHITE);
        status.setText("AETHER 3D  •  Vulkan Forward+  •  Editor");
        status.setTextSize(13);
        toolbar.addView(status, new LinearLayout.LayoutParams(0,60,1));
        Button play = new Button(this);
        play.setText("PLAY");
        play.setOnClickListener(v -> status.setText("AETHER 3D  •  PLAY MODE"));
        toolbar.addView(play, new LinearLayout.LayoutParams(130,60));
        FrameLayout.LayoutParams tp = new FrameLayout.LayoutParams(-1,70,Gravity.TOP);
        root.addView(toolbar,tp);
        setContentView(root);
    }

    @Override protected void onPause(){ super.onPause(); }
    @Override protected void onResume(){ super.onResume(); if(surface!=null) surface.resumeNative(); }

    private final class AetherSurface extends SurfaceView implements SurfaceHolder.Callback {
        AetherSurface(Context c){ super(c); getHolder().addCallback(this); setFocusable(true); }
        public void surfaceCreated(SurfaceHolder h){ nativeSetSurface(h.getSurface()); }
        public void surfaceChanged(SurfaceHolder h,int f,int w,int ht){ nativeResize(w,ht); }
        public void surfaceDestroyed(SurfaceHolder h){ nativeSetSurface(null); }
        public boolean onTouchEvent(android.view.MotionEvent e){
            nativeTouch(e.getX(),e.getY(),e.getActionMasked()); return true;
        }
        void resumeNative(){ if(getHolder().getSurface().isValid()) nativeSetSurface(getHolder().getSurface()); }
    }
}
