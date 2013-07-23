/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#ifndef SGCT_WIN_MANAGER_H_
#define SGCT_WIN_MANAGER_H_

#include <sgct.h>
namespace osp {

class Config;
class Renderer;

class SGCTWinManager {
public:
  static SGCTWinManager * New(Config *_config);
  ~SGCTWinManager();
  
  bool InitEngine();
  bool Render();

  void SetRenderer(Renderer *_renderer);

private:
  SGCTWinManager();
  SGCTWinManager(Config *_config);
  SGCTWinManager(const SGCTWinManager&);

  // Keep private, no config update in runtime
  bool UpdateConfig();

  Config *config_;
  sgct::Engine *gEngine_;
  Renderer *renderer_;

  int winWidth_;
  int winHeight_;

};

}


#endif
