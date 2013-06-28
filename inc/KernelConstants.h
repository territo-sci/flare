#ifndef KERNELCONSTANTS_H_
#define KERNELCONSTANTS_H_

/*
Author: Victor Sand (victor.sand@gmail.com)
Simple struct to gather constants used in kernel
*/

namespace osp {

struct KernelConstants {
  float stepSize;
  float intensity;
  int numBoxesPerAxis;
};

struct TraversalConstants {
  int numTimesteps_;
  int numValuesPerNode_;
  int numBSTNodesPerOT_;
  // TODO change to float
  // TODO get rid of timestep
  int timestep_;
  int temporalTolerance_;
  int spatialTolerance_;
};

}

#endif
