#ifndef TRANSFERFUNCTION_H
#define TRANSFERFUNCTION_H

#include <MappingKey.h>
#include <set>
#include <string>
#include <iostream>

namespace osp {

//class Texture1D;
	
class TransferFunction {
public:
	static TransferFunction * New();
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
	//Texture1D * Texture() { return texture_; }
	
	// TODO temp
	float * FloatData() { return floatData_; }

  // Operators
  TransferFunction& operator=(const TransferFunction &_tf);

private:
	TransferFunction();
	TransferFunction(const TransferFunction &_tf);
	float *floatData_;
	
  //Texture1D *texture_;
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
  	 
