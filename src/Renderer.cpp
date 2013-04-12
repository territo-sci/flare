#include <Renderer.h>

using namespace osp;

Renderer::Renderer() : winWidth_(0), winHeight_(0) {
}

Renderer::~Renderer() {
}

void Renderer::SetWinWidth(unsigned int _winWidth) {
  winWidth_ = _winWidth;
}

void Renderer::SetWinHeight(unsigned int _winHeight) {
  winHeight_ = _winHeight;
}
