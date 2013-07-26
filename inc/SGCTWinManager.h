/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#ifndef SGCT_WIN_MANAGER_H_
#define SGCT_WIN_MANAGER_H_

#include <sgct.h>
#include <glm/glm.hpp>

namespace osp {

class Config;
class Renderer;

class SGCTWinManager {
public:

  static SGCTWinManager * Instance();
   ~SGCTWinManager();
  
  void SetRenderer(Renderer *_renderer);
  void SetConfig(Config *_config);

  // Init engine, set up OpenGL context
  bool InitEngine(int _argc, char **_argv,
                  sgct::Engine::RunMode _runMode);
  bool Render();

  unsigned int FBOHandle() const {
    return engine_->getFBOPtr()->getBufferID();
  }

  glm::mat4 ProjMatrix() const { 
     return engine_->getActiveProjectionMatrix();
  }

  glm::mat4 ViewMatrix() const {
    return engine_->getActiveViewMatrix();
  }

private:
  SGCTWinManager();
  SGCTWinManager(const SGCTWinManager&);

  Config *config_;
  sgct::Engine *engine_;
  Renderer *renderer_;

  // For keyboard functions
  std::vector<int> keysToCheck;

  // Helper methods
  void InitNavigation();
  void UpdateNavigation();

  // Callback functions
  static void Draw();

  // Render timing variables
  static float oldTime_;
  static float currentTime_;
  static float elapsedTime_;

  // Singleton instance
  static SGCTWinManager *instance_;

};

}


#endif
