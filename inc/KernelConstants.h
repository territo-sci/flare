#ifndef KERNELCONSTANTS_H_
#define KERNELCONSTANTS_H_

/*
Author: Victor Sand (victor.sand@gmail.com)
Simple struct to gather constants used in kernel
*/

namespace osp {

typedef struct {
  float stepSize;
  float intensity;
  int numBoxesPerAxis;
} KernelConstants;

}

#endif
