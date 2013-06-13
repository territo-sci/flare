#ifndef KERNELCONSTANTS_H
#define KERNELCONSTANTS_H


/*
Author: Victor Sand (victor.sand@gmail.com)
Simple struct to gather constants used in kernel
*/

namespace osp {

typedef struct {
  float stepSize;
  float intensity;
  int xDim;
  int yDim;
  int zDim;
  int numBoxesPerAxis;
} KernelConstants;

}

#endif
