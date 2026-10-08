#include "aether_engine.h"
#include "aether_vulkan.h"
namespace aether {
Engine::Engine():renderer_(std::make_unique<VulkanRenderer>()){}
Engine::~Engine()=default;
void Engine::set_window(ANativeWindow* w){ renderer_->set_window(w); }
void Engine::resize(uint32_t w,uint32_t h){ renderer_->resize(w,h); }
void Engine::touch(float x,float y,int action){ renderer_->touch(x,y,action); }
void Engine::tick(){ renderer_->draw(); }
}
