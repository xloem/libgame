#pragma once

#include <vector>

#include "identifiers.hpp"

namespace game {

void storage_process(std::vector<uint8_t> & data, identifiers & what, bool keep_stored = true);

class storage
{
public:
	enum class process_result {
		STORED_AND_VERIFIED, // data in vector is correct and is stored reliably elsewhere
		VERIFIED,  // data in vector is correct with identifiers, no additional storage
		UNPROCESSABLE, // not enough information to process
		INCONSISTENT // data in vector is wrong or identifiers are wrong
		// if an unavoidable error is encountered, throw a process_error for now.  means processing cannot be fully completed.
	};
	storage();
	~storage();
	virtual process_result process(std::vector<uint8_t> & data, identifiers & what, bool keep_stored) = 0;
};

class process_error : public std::runtime_error
{
public:
	using std::runtime_error::runtime_error;
	// TODO: make a proper constructor
};

}
