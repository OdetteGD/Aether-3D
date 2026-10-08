#include "aether/render/forward_plus_renderer.h"

#include <android/native_window_jni.h>
#include <jni.h>
#include <memory>
#include <mutex>

namespace {
std::mutex g_mutex;
std::unique_ptr<aether::render::ForwardPlusRenderer> g_renderer;
ANativeWindow* g_window = nullptr;

void release_window() {
    if (g_window != nullptr) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }
}
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_aether3d_AetherNative_initialize(JNIEnv* env, jclass, jobject surface, jint width, jint height) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (surface == nullptr || width <= 0 || height <= 0) return JNI_FALSE;

    if (g_renderer) {
        g_renderer->shutdown();
        g_renderer.reset();
    }
    release_window();

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) return JNI_FALSE;

    auto renderer = std::make_unique<aether::render::ForwardPlusRenderer>();
    if (!renderer->initialize(window, static_cast<uint32_t>(width), static_cast<uint32_t>(height))) {
        renderer->shutdown();
        ANativeWindow_release(window);
        return JNI_FALSE;
    }

    g_window = window;
    g_renderer = std::move(renderer);
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_aether3d_AetherNative_resize(JNIEnv*, jclass, jint width, jint height) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_renderer || width <= 0 || height <= 0) return JNI_FALSE;
    return g_renderer->resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height)) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_aether3d_AetherNative_shutdown(JNIEnv*, jclass) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_renderer) {
        g_renderer->shutdown();
        g_renderer.reset();
    }
    release_window();
}
