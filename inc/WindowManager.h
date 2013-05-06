#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

/* 
 * Author: Victor Sand (victor.sand@gmail.com)
 * Class to reasonably abstract Window handling away from rendering.
 *
 */

#include <string>
#include <map>

namespace osp {
  
class Renderer;

class WindowManager {
public:
  static WindowManager * New(unsigned int _width,
                             unsigned int _height,
                             std::string _title);
  bool OpenWindow();
  bool StartLoop();
  bool Close();
  void SetTitle(std::string _title);
  void SetRenderer(Renderer *_renderer);

  unsigned int Width() const { return width_; }
  unsigned int Height() const { return height_; }

private:
  WindowManager(unsigned int _width, unsigned int _height, std::string _title);
  bool Draw(float _timeStep);
  bool windowOpen_;

  unsigned int width_;
  unsigned int height_;
  std::string title_;
  Renderer *renderer_;
};

}

#endif
