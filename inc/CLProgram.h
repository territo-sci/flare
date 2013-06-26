/*
 * Victor Sand (victor.sand@gmail.com)
 *
 */

#ifndef CL_PROGRAM_H_
#define CL_PROGRAM_H_

#include <CL/cl.hpp>
#include <map>
#include <string>

namespace osp {

class CLManager;
class TransferFunction;
class Texture;
struct KernelConstants;

class CLProgram {
public:
  static CLProgram * New(const std::string &_programName, 
                         CLManager *_clManager);
  ~CLProgram();

  struct MemArg {
    size_t size_;
    cl_mem mem_;
  };

  cl_program Program() { return program_; }
  cl_kernel Kernel() { return kernel_; }
  cl_int Error() { return error_; }

  bool CreateProgram(std::string _fileName);
  bool BuildProgram();
  bool CreateKernel();

  bool AddTexture(unsigned int _argNr, Texture *_texture,
                  GLuint _textureType,
                  cl_mem_flags _permissions);

  bool AddTransferFunction(unsigned int _argNr,
                           TransferFunction *_transferFunction);

  bool AddKernelConstants(unsigned int _argNr, 
                          KernelConstants *_kernelConstants);

  bool AddIntArray(unsigned int _argNr, int *_intArray, unsigned int _size,
                   cl_mem_flags _permissions);

  bool PrepareProgram();
  bool LaunchProgram();
  bool FinishProgram();

private:
  CLProgram(const std::string &_programName, CLManager *_clManager);
  CLProgram();
  CLProgram(const CLProgram&);

  char * ReadSource(std::string _fileName, int &_numChars);

  std::string programName_;

  CLManager *clManager_;
  cl_program program_;
  cl_kernel kernel_;
  cl_int error_;
  // Stores device OGL textures together with their kernel arg nummer
  std::map<cl_uint, cl_mem> OGLTextures_;
  // Stores non-texture memory buffer arguments
  std::map<cl_uint, MemArg> memArgs_;

};

}

#endif
