#ifndef TRANSFERFUNCTION_H
#define TRANSFERFUNCTION_H

#include <MappingKey.h>
#include <set>
#include <string>
#include <iostream>

namespace osp {

class Texture1D;
	
class TransferFunction {
public:
	TransferFunction();
	TransferFunction(const TransferFunction &_tf);
	~TransferFunction();
	
	enum Interpolation {
		LINEAR = 0,
	};

	// Take the saved values and construct a texture
	bool ConstructTexture();
	// Read TF from the in file
	bool ReadFile();
	std::string ToString() const;
  
  // Mutators
	void SetInFilename(const std::string &_inFilename);

	// Accessors
	unsigned int Width() const { return width_; }
	Texture1D * Texture() { return texture_; }

  // Operators
  TransferFunction& operator=(const TransferFunction &_tf);

private:
  Texture1D *texture_;
	unsigned int width_;
	float lower_;
	float upper_;
	bool generatedTexture_;
	std::string inFilename_;
	std::set<MappingKey> mappingKeys_;
	Interpolation interpolation_;
};

}

std::ostream & operator<<(std::ostream &os, const osp::TransferFunction &_tf);

#endif
  	 
