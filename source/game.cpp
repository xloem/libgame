#include <game/identifiers.hpp>
#include <game/utilities.hpp>
#include <game/storage.hpp>

#include <iostream>

//#include <bitcoin/bitcoin.hpp>
//#include <bitcoin/client.hpp>
//using namespace libbitcoin;

using namespace std;
using namespace game;


int main()
{
	string test_string = "Have fun!";
	vector<uint8_t> test_data(test_string.begin(), test_string.end());

#if 0

	// note: there are ways to upload data to bitcoin more cheaply than op_return.  op_return will be implemented first for simplicity.

	// 0. transaction
	chain::transaction tx;
	tx.set_version(1);

	// 1. private key
	wallet::ec_private private_key(base16_literal("00ffcfe1d3810dff2a69d676ee0ddf286d441f00595f20f8d89d84e50fabd060"));

	// 2. utxo
	hash_digest utxo_hash = hash_literal("9691d56dea012fe6d7637c06de39ee3a70457e5b145628c67b4ac85be50edafe");
	chain::output_point utxo_point(utxo_hash, 9);
	chain::script utxo_script = chain::script::to_pay_key_hash_pattern(private_key.to_payment_address().hash());
	chain::input input_from_utxo(utxo_point, utxo_script, 0xffffffff);

	tx.inputs().push_back(input_from_utxo);

	uint64_t utxo_satoshis = 100000;

	// 3. change
	wallet::payment_address change_address{private_key};

	chain::script changeScript = chain::script::to_pay_key_hash_pattern(change_address.hash());
	chain::output changeOutput(utxo_satoshis, changeScript);

	tx.outputs().push_back(changeOutput);

	// 4. sign
	endorsement signature;
	if (!utxo_script.create_endorsement(signature, private_key.secret(), utxo_script, tx, 0, machine::all)) {
		std::cerr << "Failed to sign" << std::endl;
		return -1;
	}
	machine::operation::list signature_script;
	signature_script.push_back(machine::operation(signature));
	signature_script.push_back(machine::operation(to_chunk(private_key.to_public().point())));
	chain::script unlocking_script(signature_script);

	tx.inputs()[0].set_script(unlocking_script);

	std::cout << "Transaction: " << encode_base16(tx.to_data()) << std::endl;



	// Send
	client::connection_type connection;
	connection.retries = 3;
	connection.timeout_seconds = 1;
	connection.server = config::endpoint("tcp://mainnet1.libbitcoin.net:19091");

	client::obelisk_client client(connection);
	if (!client.connect(connection)) {
		std::cout << "failed to connect" << std::endl;
		return 0;
	}

	client.transaction_pool_broadcast([](const code & ec) {
			std::cout << "error: " << ec.message() << std::endl;
		}, [](const code & ec) {
			std::cout << "success: " << ec.message() << std::endl;
		}, tx);
	client.wait();





	/*game::identifiers identifiers;

	storage_process(test_data, identifiers);

	std::cout << test_string << std::endl;
	for (auto identifier : identifiers) {
		std::cout << identifier.first << ": " << identifier.second << std::endl;
	}*/
#endif

	return 0;
}
