#pragma once

#include <chrono>

namespace game {

using seconds = double;

seconds time()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / double(1000000);
}

}
