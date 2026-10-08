#include "aether_engine.h"
#include <jni.h>
#include <android/native_window_jni.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
static aether::Engine g_engine;
static std::thread g_thread;
static std::mutex g_mutex;
static std::atomic<bool> g_running{false};
static void stop_render(){ g_running=false; if(g_thread.joinable())g_thread.join(); }
static void start_render(){
    if(g_running.exchange(true))return;
    g_thread=std::thread([]{ while(g_running){ {std::lock_guard<std::mutex> lock(g_mutex); g_engine.tick(); } std::this_thread::sleep_for(std::chrono::milliseconds(8)); }});
}
extern "C" JNIEXPORT void JNICALL Java_com_odettegd_aether3d_MainActivity_nativeSetSurface(JNIEnv* env,jobject,jobject surface){
    std::lock_guard<std::mutex> lock(g_mutex);
    if(!surface){ stop_render(); g_engine.set_window(nullptr); return; }
    ANativeWindow* w=ANativeWindow_fromSurface(env,surface); g_engine.set_window(w); ANativeWindow_release(w); start_render();
}
extern "C" JNIEXPORT void JNICALL Java_com_odettegd_aether3d_MainActivity_nativeResize(JNIEnv*,jobject,jint w,jint h){
    std::lock_guard<std::mutex> lock(g_mutex); g_engine.resize((uint32_t)w,(uint32_t)h);
}
extern "C" JNIEXPORT void JNICALL Java_com_odettegd_aether3d_MainActivity_nativeTouch(JNIEnv*,jobject,jfloat x,jfloat y,jint action){
    std::lock_guard<std::mutex> lock(g_mutex); g_engine.touch(x,y,action);
}
