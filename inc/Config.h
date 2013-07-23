/* 
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 * Simple functionality to read, save and access constants.
 * Reads the specified file as soon as the object is created. 
 *
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>

namespace osp {

class Config {
public:
  static Config * New(const std::string &_configFilename);
  ~Config();

  // Reads the config file, can be called by external modules
  bool Read();

  int WinWidth() const { return winWidth_; }
  int WinHeight() const { return winHeight_; }
  std::string TSPFilename() const { return TSPFilename_; }
  std::string TFFilename() const { return TFFilename_; }
  float SpatialErrorTolerance() const { return spatialErrorTolerance_; }
  float TemporalErrorTolerance() const { return temporalErrorTolerance_; }
  float TSPTraversalStepsize() const { return TSPTraversalStepsize_; }
  float RaycasterStepsize() const { return raycasterStepsize_; }
  float RaycasterIntensity() const { return raycasterIntensity_; }
  float AnimatorRefreshInterval() const { return animatorRefreshInterval_; }

private:
  Config();
  Config(const std::string &_configFilename);
  Config(const Config&);

  std::string configFilename_;

  int winWidth_;
  int winHeight_;
  std::string TSPFilename_;
  std::string TFFilename_;
  float spatialErrorTolerance_;
  float temporalErrorTolerance_;
  float TSPTraversalStepsize_;
  float raycasterStepsize_;
  float raycasterIntensity_;
  float animatorRefreshInterval_;
};

}

#endif


