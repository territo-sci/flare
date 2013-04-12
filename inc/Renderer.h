#ifndef RENDERER_H
#define RENDERER_H

namespace osp {

class Renderer {
public:
  virtual bool Render(float _timestep) = 0;
  virtual bool ReloadShaders() = 0;
  virtual bool ReloadConfig() = 0;
  virtual ~Renderer();
  void SetWinWidth(unsigned int _winWidth);
  void SetWinHeight(unsigned int _winHeight);
protected:
  Renderer();
  unsigned int winWidth_;
  unsigned int winHeight_;
};

}

#endif