#include <game/storage.hpp>

#include <string>
#include <unordered_set>

using namespace std;
using namespace game;

static unordered_set<storage *> storage_all;

using process_result = storage::process_result;

static void process_result_propagate(process_result this_result, bool keep_stored, process_result & result)
{
	switch (this_result) {
	case process_result::UNPROCESSABLE:
		return;
		break;
	case process_result::INCONSISTENT:
		throw process_error("data inconsistent with identifiers");
		break;
	case process_result::VERIFIED:
		if (process_result::UNPROCESSABLE == result) {
			result = process_result::VERIFIED;
		}
		break;
	case process_result::STORED_AND_VERIFIED:
		if (!keep_stored) { throw process_error("data was stored"); }
		result = process_result::STORED_AND_VERIFIED;
		break;
	default:
		throw std::logic_error("corruption in enum switch case");
	}
}

void game::storage_process(std::vector<uint8_t> & data, identifiers & what, bool keep_stored)
{
	process_result result;
	for (auto & backend : storage_all) {
		result = backend->process(data, what, keep_stored);
		if (result != process_result::UNPROCESSABLE && data.size()) {
			break;
		}
	}
	if (result == process_result::UNPROCESSABLE) { throw process_error("data not processed"); }
	process_result_propagate(result, keep_stored, result);
	for (auto & backend : storage_all) {
		auto this_result = backend->process(data, what, keep_stored);
		process_result_propagate(this_result, keep_stored, result);
	}
	switch (result) {
	case process_result::UNPROCESSABLE:
		throw process_error("data not processed");
	case process_result::VERIFIED:
		if (keep_stored) {
			throw process_error("data not stored");
		} else {
			return;
		}
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
