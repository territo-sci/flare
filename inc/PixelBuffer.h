#ifndef PIXELBUFFER_H_
#define PIXELBUFFER_H_

/*
 * Author: Victor Sand (victor.sand@gmail.com)
 * Wrapper for OpenGL PBO
 * TODO only supports float data for now
 *
 */

namespace osp {

class PixelBuffer {
public:
  static PixelBuffer * New(unsigned int _numFloats);
  
  // Create and set up buffer, but don't copy any data
  bool Init();
  // Fill PBO with float data
  bool Update(float *_data);

  unsigned int Handle() const { return handle_; }

private:
  PixelBuffer();
  PixelBuffer(unsigned int _numFloats);
  PixelBuffer(const PixelBuffer&) {}

  unsigned int handle_;
  bool initialized_;
  unsigned int numFloats_;
};

}

#endif
