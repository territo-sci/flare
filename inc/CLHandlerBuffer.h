/*
 * Author: Victor Sand (victor.sand@gmail.com
 *
 * CLHandler version that uses pinned memory and float
 * buffers for data transfers.
 */

#ifndef CL_HANDLERBUFFER_H_
#define CL_HANDLERBUFFER_H_

#include <CLHandler.h>

namespace osp {

class CLHandlerBuffer : public CLHandler {
public:
  static CLHandlerBuffer * New();
  ~CLHandlerBuffer();

  virtual bool InitBuffers(unsigned int _argNr,
                           VoxelData<float> *_voxelData);

  virtual bool UpdateHostMemory(MemIndex _memIndex,
                                VoxelData<float> *_data,
                                unsigned int _timestep);
  
  virtual bool WriteToDevice(MemIndex _memIndex);

  virtual bool PrepareRaycaster();

private:
  CLHandlerBuffer();

  // Page-locked memory on host
  std::vector<MemArg> pinnedHostMemory_;
  // Device buffers in which to load into from pinned memory
  std::vector<MemArg> deviceBuffers_;
  // Mapped pointers, referencing the pinned host memory
  std::vector<float*> pinnedPointers_;
  // Size of buffers for float data
  size_t bufferSize_;

  // Size of copy buffer
  // TODO Do a "dry run" at init and determine this value
  static const unsigned int copyBufferSize_ = 262144;

};

}


#endif
