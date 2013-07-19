/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <Config.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <Utils.h>

using namespace osp;

Config::Config() : configFilename_("OopsNotSet") {}

Config::Config(const std::string &_configFilename) 
  : configFilename_(_configFilename) {}

Config::~Config() {}

Config * Config::New(const std::string &_configFilename) {
  Config *config = new Config(_configFilename);
  if (!config->Read()) {
    ERROR("Could not read config. Returning NULL.");
    delete config;
    return NULL;
  }
  return config;
}

bool Config::Read() {
  
  INFO("\nReading config from " << configFilename_);

  std::ifstream in;
  in.open(configFilename_.c_str(), std::ifstream::in);
  if (!in.is_open()) {
    ERROR("Could not open " << configFilename_);
    return false;
  }

  std::string line;
  while (std::getline(in, line)) {
    // Ignore empty lines and comments
    if (!line.empty() && line.at(0) != '#') {
      // Read variable name
      std::stringstream ss;
      ss.str(line);
      std::string variable;
      ss >> variable;
      // Save value
      if (variable == "tsp_filename") {
        ss >> TSPFilename_;
        INFO("TSP file name: " << TSPFilename_);
      } else if (variable == "transferfunction_filename") {
        ss >> TFFilename_; 
        INFO("Transfer function file name " << TFFilename_);
      } else if (variable == "spatial_error_tolerance") {
        ss >> spatialErrorTolerance_;
        INFO("Spatial error tolerance: " << spatialErrorTolerance_);
      } else if (variable == "temporal_error_tolerance") {
        ss >> temporalErrorTolerance_;
        INFO("Temporal error tolerance: " << temporalErrorTolerance_);
      } else if (variable == "tsp_traversal_stepsize") {
        ss >> TSPTraversalStepsize_;
        INFO("TSP traversal step size: " << TSPTraversalStepsize_); 
      } else if (variable == "raycaster_stepsize") {
        ss >> raycasterStepsize_; 
        INFO("Ray caster step size: " << raycasterStepsize_);
      } else if (variable == "raycaster_intensity") {
        ss >> raycasterIntensity_;
        INFO("Ray caster intensity: " << raycasterIntensity_);
      } else if (variable == "animator_refresh_interval") {
        ss >> animatorRefreshInterval_;
        INFO("Animator refresh interval: " << animatorRefreshInterval_);
      } else if (variable == "win_width") {
        ss >> winWidth_;
        INFO("Win width: " << winWidth_);
      } else if (variable == "win_height") {
        ss >> winHeight_;
        INFO("Win height: " << winHeight_);
      } else { 
        ERROR("Variable name " << variable << " unknown");
      }
    }
  }

  INFO("");

  return true;
}





