#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include <unistd.h>

#include "old/skystream.hpp"

int main(int argc, char **argv)
{
	sia::portalpool pool;
	skystream stream(pool);

	unsigned long long offset = 0;

	std::vector<uint8_t> data;
	data.reserve(1024 * 1024 * 16);
	data.resize(data.capacity());

	ssize_t size;

	while ((size = read(0, data.data(), data.size()))) {
		if (size < 0) {
			perror("read");
			std::cerr << stream.identifiers().dump(2) << std::endl;
			return size;
		}
		data.resize(size);
		stream.write(data, "bytes", offset);
		offset += data.size();
		data.resize(data.capacity());
	}
	std::cout << stream.identifiers().dump(2) << std::endl;
	return 0;
}
