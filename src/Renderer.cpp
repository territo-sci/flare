#include <Renderer.h>

using namespace osp;

Renderer::Renderer() : winWidth_(0), 
                       winHeight_(0),
                       currentMouseX_(0),
                       currentMouseY_(0),
                       lastMouseX_(0),
                       lastMouseY_(0),
                       leftMouseDown_(false),
                       rightMouseDown_(false) {
}

Renderer::~Renderer() {
}

void Renderer::SetWinWidth(unsigned int _winWidth) {
  winWidth_ = _winWidth;
}

void Renderer::SetWinHeight(unsigned int _winHeight) {
  winHeight_ = _winHeight;
}

void Renderer::SetMousePosition(float _mouseX, float _mouseY) {
  lastMouseX_ = currentMouseX_;
  lastMouseY_ = currentMouseY_;
  currentMouseX_ = _mouseX;
  currentMouseY_ = _mouseY;
}

void Renderer::SetMousePressed(bool _leftPressed, bool _rightPressed) {
  leftMouseDown_ = _leftPressed;
  rightMouseDown_ = _rightPressed;
}

void Renderer::SetKeyPressed(std::string _key, bool _pressed) {
  std::map<std::string, bool>::iterator it;
  it = keysPressed_.find(_key);
  if (it == keysPressed_.end()) {
    keysPressed_.insert(make_pair(_key, _pressed));
  } else {
    it->second = _pressed;
  }
}

bool Renderer::KeyPressed(std::string _key) const {
  std::map<std::string, bool>::const_iterator it;
  it = keysPressed_.find(_key);
  if (it == keysPressed_.end()) {
    return false;
  } else {
    return it->second;
  }
}