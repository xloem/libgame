#include <game/storage.hpp>

#include <cstring>

#include <siaskynet.hpp>

using namespace sia;

static class siaskynet : public game::storage
{
public:
	siaskynet()
	: portals(skynet::portals()),
	  portal(portals.front())
	{ }

	virtual process_result process(std::vector<uint8_t> & data, game::identifiers & what, bool keep_stored) override
	{
		// this function needs simplification.
		// maybe start by pulling out handling mirrors to another function or class.
		// want to upload to >1 mirror to provide more leeway if a mirror stops paying for a file
		// TODO: reupload after some time?
		
		if (what.size() == 0) { return process_result::UNPROCESSABLE; }
		if (!what.count("skylink")) {
			if (!data.size() || !keep_stored) {
				return process_result::UNPROCESSABLE;
			}
			size_t count = 0;
			std::string identifier;
			for (auto & options : portals) {
				portal.options = options;
				if (count >= 2) { break; }
				auto new_identifier = portal.upload(what.begin()->second, data);
				if (!identifier.size()) { identifier = new_identifier; }
				if (new_identifier != identifier) {
					identifier = new_identifier;
					count = 1;
				} else {
					++ count;
				}
			}
			if (count < 2) {
				throw game::process_error("failed to upload to sia skynet");
			}
			what["skylink"] = identifier;
		}
		auto remote_data = portal.download(what["skylink"]);
		if (!data.size()) {
			data = remote_data.data;
			return process_result::STORED_AND_VERIFIED;
		}
		if (remote_data.data != data) {
			what.erase("skylink");
			return process_result::INCONSISTENT;
		}
		return process_result::STORED_AND_VERIFIED;
	}
private:
	
	decltype(skynet::portals()) portals;
	skynet portal;
} storage_siaskynet;
