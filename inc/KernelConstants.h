#ifndef KERNELCONSTANTS_H_
#define KERNELCONSTANTS_H_

/*
Author: Victor Sand (victor.sand@gmail.com)
Simple struct to gather constants used in kernel
*/

namespace osp {

struct KernelConstants {
  float stepsize_;
  float intensity_;
  int numTimesteps_;
  int numValuesPerNode_;
  int numOTNodes_;
  int numBoxesPerAxis_;
  int timestep_;
  int temporalTolerance_;
  int spatialTolerance_;
  int rootLevel_;
  int paddedBrickDim_;
};

struct TraversalConstants {
  int numTimesteps_;
  int numValuesPerNode_;
  int numOTNodes_;
  // TODO change to float
  // TODO get rid of timestep
  int timestep_;
  int temporalTolerance_;
  int spatialTolerance_;
};

}

#endif
