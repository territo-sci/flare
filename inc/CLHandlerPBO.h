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

class CLHandlerPBO : public CLHandler {
public:
  static CLHandlerPBO * New();
  ~CLHandlerPBO();

  virtual bool PrepareRaycaster();

  virtual bool InitBuffers(unsigned int _argNr,
                           VoxelData<float> *_voxelData);

  virtual bool UpdateHostMemory(MemIndex _memIndex,
                                VoxelData<float> *_data,
                                unsigned int _timestep);

  virtual bool WriteToDevice(MemIndex _index);

private:

  // Pixel buffers 
  std::vector<PixelBuffer*> pixelBuffers_;
  // Host side textures
  std::vector<Texture2D*> hostTextures_;
  // Device texture buffers in which to load into from PBOs
  std::vector<MemArg> deviceTextures_;

};

}

#endif
