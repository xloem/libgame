#include <game/storage.hpp>

#include <string>
#include <unordered_set>

using namespace std;
using namespace game;

static unordered_set<storage *> storage_all;

void game::storage_process(std::vector<uint8_t> & data, identifiers & what)
{
	using process_result = storage::process_result;
	process_result result;
	for (auto & backend : storage_all) {
		result = backend->process(data, what);
		if (result != process_result::UNPROCESSABLE) { break; }
	}
	if (result == process_result::UNPROCESSABLE) { throw process_error("data not processed"); }
	result = process_result::UNPROCESSABLE;
	for (auto & backend : storage_all) {
		auto this_result = backend->process(data, what);
		switch (this_result) {
		case process_result::UNPROCESSABLE:
			continue;
			break;
		case process_result::INCONSISTENT:
			throw process_error("data inconsistent with identifiers");
			break;
		case process_result::VERIFIED:
			if (result == process_result::UNPROCESSABLE) {
				result = process_result::VERIFIED;
			}
			break;
		case process_result::STORED_AND_VERIFIED:
			result = process_result::STORED_AND_VERIFIED;
			break;
		default:
			throw std::logic_error("corruption in enum switch case");
		}
	}
	switch (result) {
	case process_result::UNPROCESSABLE:
		throw process_error("data not processed");
	case process_result::VERIFIED:
		throw process_error("data not stored");
	case process_result::STORED_AND_VERIFIED:
		return;
	default:
		throw std::logic_error("corruption in enum switch case");
	}
}

storage::storage()
{
	storage_all.insert(this);
}

storage::~storage()
{
	storage_all.erase(this);
}
