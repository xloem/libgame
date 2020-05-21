#include <game/identifiers.hpp>
#include <game/utilities.hpp>
#include <game/storage.hpp>

#include <iostream>

using namespace std;
using namespace game;

int main()
{
	string test_string = "Have fun!";
	vector<uint8_t> test_data(test_string.begin(), test_string.end());

	game::identifiers identifiers;

	storage_process(test_data, identifiers);

	std::cout << test_string << std::endl;
	for (auto identifier : identifiers) {
		std::cout << identifier.first << ": " << identifier.second << std::endl;
	}

	return 0;
}
