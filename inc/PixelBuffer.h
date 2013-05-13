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
  
  // Create and set up buffer
  bool Init(float *_data);
  // Map the pointer to the PBO 
  bool MapPointer();
  // Unmap the PBO pointer
  bool UnmapPointer();
  // Update once the pointer has been mapped. This function can be called
  // from a separate thread as it does not mess with the OpenGL context.
  bool Update(float *_data);
  unsigned int Handle() const { return handle_; }

private:
  PixelBuffer();
  PixelBuffer(unsigned int _numFloats);
  PixelBuffer(const PixelBuffer&) {}

  float *mappedPointer_;
  unsigned int handle_;
  bool initialized_;
  unsigned int numFloats_;
};

}

#endif
