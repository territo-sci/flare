#include <TransferFunction.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <Utils.h>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <math.h>

using namespace osp;

TransferFunction::TransferFunction() : 
	texture_(NULL),
  width_(0),
	lower_(0.f),
  upper_(1.f),
  generatedTexture_(false),
  inFilename_("NotSet"),
  interpolation_(TransferFunction::LINEAR) {}


TransferFunction::TransferFunction(const TransferFunction &_tf) {
	// TODO
}


TransferFunction::~TransferFunction() {}

bool TransferFunction::ReadFile() {
	std::ifstream in;
	in.open(inFilename_.c_str());
	if (!in.is_open()) {
		ERROR("Could not open file " << inFilename_);
		return false;
	}
	std::string line;

	while (std::getline(in, line)) {
		boost::char_separator<char> sep(" ");
		boost::tokenizer<boost::char_separator<char> > tokens(line, sep);

		boost::tokenizer<boost::char_separator<char> >::iterator it=tokens.begin();
		float intensity;
		unsigned int r, g, b, a;
		if (*it == "width") {
			width_ = boost::lexical_cast<unsigned int>(*(boost::next(it)));
		} else if (*it == "lower") {
			lower_ = boost::lexical_cast<float>(*(boost::next(it)));
		} else if (*it == "upper") {
			upper_ = boost::lexical_cast<float>(*(boost::next(it)));
		} else if (*it == "mappingkey") {
			intensity = boost::lexical_cast<float>(*(++it));
			r = boost::lexical_cast<unsigned int>(*(++it));
			g = boost::lexical_cast<unsigned int>(*(++it));
			b = boost::lexical_cast<unsigned int>(*(++it));
			a = boost::lexical_cast<unsigned int>(*(++it));
			MappingKey mp(intensity, r, g, b, a);
			mappingKeys_.insert(mp);
		} else {
			ERROR("Parsing error");
			return false;
		}
	}
	 
	in.close();
	return true;
}

void TransferFunction::SetInFilename(const std::string &_inFilename) {
	inFilename_ = _inFilename;
}

std::string TransferFunction::ToString() const {
	std::stringstream ss;
	ss << "Width: " << width_ << "\n";
	ss << "Range: [" << lower_ << " " << upper_ << "]\n";
	ss << "Number of mapping keys: " << mappingKeys_.size(); 
	for (std::set<MappingKey>::iterator it = mappingKeys_.begin();
		   it != mappingKeys_.end();
			 it++) {
		ss << "\n" << *it;
	}
  return ss.str();
}

bool TransferFunction::ConstructTexture() {

	if (mappingKeys_.empty()) {
		ERROR("No mapping keys");
		return false;
	}

	if (interpolation_ == TransferFunction::LINEAR) {

		// Float values for R, G, B and A channels
		float *values = new float[4*width_];
		
		unsigned int lowerIndex = (unsigned int)floorf(lower_*(float)width_);
		unsigned int upperIndex = (unsigned int)floorf(upper_*(float)width_); 

		// Loop over channels
		for (unsigned int channel=0; channel<4; channel++) {

			// Init some mapping keys
			std::set<MappingKey>::iterator it = mappingKeys_.begin();
			MappingKey prev(lower_, 0, 0, 0, 0);
			MappingKey next = *it;
			MappingKey last(upper_, 0, 0, 0, 0);
			// First mapping key switch is a special case
			bool first = true;

			for (unsigned int i=0; i<width_; i++) {

				if (i <= lowerIndex || i > upperIndex) {
					values[4*i + channel] = 0.f;
				} else {

					// See if it's time to go to next pair of mapping keys
					float pos = (float)i/width_;
					if (!first && pos > next.Intensity()) {
						prev = next;
						it++;
						if (it == mappingKeys_.end()) {
							next = last;
						} else {
							next = *it;
						}
					}

					first = false;

					// Interpolate linearly between prev and next mapping key		
				  DEBUG("i: " << i);
					DEBUG("pos: " << pos);
					DEBUG("prev.Intensity(): " << prev.Intensity());
					float dist = pos-prev.Intensity();
					DEBUG("dist: " << dist);
					float weight = dist/(next.Intensity()-prev.Intensity());
					DEBUG("weight: " << weight);
					values[4*i + channel] = ((float)prev.Channel(channel)*(1.f-weight)+ 
					                        (float)next.Channel(channel)*weight)/255.0;

				}
			}
		}

		// test
		for (unsigned int i=0; i<width_; i++) {
			DEBUG(i << " " << values[4*i]);
		}


		delete[] values;
	} else {
		ERROR("Invalid interpolation mode");
		return false;
	}

	return true;

}

std::ostream & operator<<(std::ostream &os, const TransferFunction &_tf) {
	os << _tf.ToString(); 
  return os;
}

