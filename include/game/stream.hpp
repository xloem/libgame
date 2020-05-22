#pragma once

#include "identifiers.hpp"

namespace game{

using offset_t = double;

class stream
{
public:
	stream();
	stream(game::identifiers & identifiers);

	void read(std::vector<uint8_t> & data, offset_t offset, std::string span = "bytes"/*, std::string flow = "real"*/);
};
	void write(std::vector<uint8_t> const & data, offset_t offset = -1, std::string span = "bytes"/*, std::string flow = "real"*/);

	std::map<std::string, std::pair<offset_t, offset_t>> chunk_spans(std::string span, offset_t offset/*, std::string flow = "real"*/);

	std::pair<offset_t, offset_t> chunk_span(std::string span, offset_t offset/*, std::string flow = "real"*/);

	std::map<std::string,std::pair<offset_t, offset_t>> spans(/*std::string flow = "real"*/);

	std::pair<offset_t, offset_t> span(std::string span/*, std::string flow = "real"*/);

	std::map<std::string, offset_t> lengths(/*std::string flow = "real"*/);

	offset_t length(std::string span/*,std::string flow = "real"*/);

	game::identifiers identifiers();

private:
	struct node;

	node * tail;
	std::unordered_map<std::string, std::unique_ptr<node>> cache;
}
