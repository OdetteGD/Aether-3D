#pragma once
#include <cstdint>
#include <memory>
struct ANativeWindow;
namespace aether {
class VulkanRenderer;
class Engine {
public:
    Engine();
    ~Engine();
    void set_window(ANativeWindow* window);
    void resize(uint32_t w, uint32_t h);
    void touch(float x, float y, int action);
    void tick();
private:
    std::unique_ptr<VulkanRenderer> renderer_;
};
}
