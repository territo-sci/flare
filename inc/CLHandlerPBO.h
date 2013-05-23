/*
 *
 * Author: Victor Sand (victor.sand@gmail.com)
 * 
 * CLHandler version that use PBO's and textures for data transfers.
 *
 */

#ifndef CL_HANDLERPBO_H_
#define CL_HANDLERPBO_H_

#include <CLHandler.h>

namespace osp {

class PixelBuffer;
class Texture3D;

class CLHandlerPBO : public CLHandler {
public:
  static CLHandlerPBO * New();
  ~CLHandlerPBO();

  virtual bool PrepareRaycaster();
  virtual bool FinishRaycaster();

  virtual bool InitBuffers(unsigned int _argNr,
                           VoxelData<float> *_voxelData);

  virtual bool UpdateHostMemory(MemIndex _memIndex,
                                VoxelData<float> *_data,
                                unsigned int _timestep);

  virtual bool WriteToDevice(MemIndex _index);

private:
  CLHandlerPBO();

  // Handles to PBOs
  std::vector<unsigned int>  pixelBufferObjects_;
  // Host side textures
  std::vector<Texture3D*> hostTextures_;
  // Device texture buffers in which to load into from PBOs
  std::vector<MemArg> deviceTextures_;

};

}

#endif
