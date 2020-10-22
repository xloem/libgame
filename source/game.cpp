#include <game/identifiers.hpp>
#include <game/utilities.hpp>
#include <game/storage.hpp>

#include <iostream>

#include <bitcoin/system.hpp>
#include <bitcoin/client.hpp>
using namespace libbitcoin;

using namespace std;
using namespace game;

class ashesofshreddeddocument
{
/*private:
	template <typename Dispatch, typename... Args>
	auto libbitcoinasync(std::string information, Dispatch dispatch, Args... args) {
	
		libbitcoin::code bc_error;
		error_handler on_error = [&bc_error](const libbitcoin::code & error)
		{
			bc_error = error;
		};

		typedef decltype(returnresult(dispatch)) Result;
		Result bc_result;

		result_handler<Result> on_done = [&bc_result](const Result & result)
		{
			bc_result = result;
		};
	
		((client).*(dispatch))(on_error, on_done, args...);
		client.wait();
		if (bc_error) {
			throw std::system_error(bc_error, information);
		}
		return bc_result;
	};*/

public:
	static std::string new_unencrypted_user()
	{
		std::vector<uint8_t> seed(64);
		std::cerr << "Warning: key made with predictable pseudo-random numbers from clock." << std::endl;
		libbitcoin::system::pseudo_random::fill(seed);
		libbitcoin::system::wallet::ec_private key(seed);
		return key.encoded();
	}

	static std::string encrypt_user(std::string unencrypted, std::string passphrase)
	{
		libbitcoin::system::wallet::ec_private clearkey(unencrypted);
		libbitcoin::system::wallet::encrypted_private crypt;
		libbitcoin::system::wallet::encrypt(crypt, clearkey.secret(), passphrase, clearkey.version(), clearkey.compressed());
		libbitcoin::system::wallet::ek_private cryptkey(crypt);
		return cryptkey.encoded();
	}

	static std::string decrypt_user(std::string encrypted, std::string passphrase)
	{
		libbitcoin::system::wallet::ek_private cryptkey(encrypted);
		libbitcoin::system::ec_secret clear;
		uint8_t version;
		bool compressed;
		libbitcoin::system::wallet::decrypt(clear, version, compressed, cryptkey, passphrase);
		libbitcoin::system::wallet::ec_private clearkey(clear, version, compressed);
		return clearkey.encoded();
	}

	static std::string new_encrypted_user(std::string passphrase)
	{
		return encrypt_user(new_unencrypted_user(), passphrase);
	}

	// 19091 appears to be testnet, even if the hostname says mainnet
	//ashesofshreddeddocument(std::string user, std::string server = "tcp://mainnet1.libbitcoin.net:19091")
	ashesofshreddeddocument(std::string user, std::string server = "tcp://mainnet.libbitcoin.net:9091")
	: client(/*retries*/3)
	{
		//#if 0
		//libbitcoin::ec_secret secret;
		//libbitcoin::decode_base16(secret, user);
		privkey = libbitcoin::system::wallet::ec_private(user);//secret);
		if (privkey) {
			libbitcoin::system::ec_compressed pub;
			libbitcoin::system::secret_to_public(pub, privkey.secret());
			address = privkey;
		} else {
			address = user;
		}
		paymentkey = libbitcoin::system::sha256_hash(address.output_script().to_data(false));


		if (!client.connect(libbitcoin::system::config::endpoint(server))) {
			throw std::runtime_error("failed to connect to " + server);
		}

		//libbitcoinasync("subscribe address", &Client::subscribe_address, address);
		client.subscribe_key(std::bind(&ashesofshreddeddocument::on_update, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), paymentkey);

		// get all utxos
		utxos = libbitcoinasync<libbitcoin::system::chain::points_value>("fetch unspent outputs", &Client::blockchain_fetch_unspent_outputs, paymentkey, /*satoshi needed*/1, libbitcoin::system::wallet::select_outputs::algorithm::individual).points;

		//std::cout << user << std::endl;
		//std::cout << privkey << std::endl;
		std::cout << "Address: " << address << std::endl;
		for (auto point : utxos) {
			std::cout << libbitcoin::system::encode_hash(point.hash()) << ":" << point.index() << " " << point.value() << " " << point.is_null() << std::endl;
		}
		//#endif
	}

	std::string process(std::vector<uint8_t> const & data, std::vector<std::array<uint8_t, 20>> const & categories)
	{
		// categories as utxos might bump into dust limit
		///*
		
		libbitcoin::system::chain::transaction tx;
		tx.set_version(1);

		// note: there are ways to upload data to bitcoin more cheaply than op_return (multisig output maybe?).  op_return will be implemented first for simplicity.
		
		double fee_per_byte = 1;
		double fee_per_sigop = 100;

		
		libbitcoin::system::chain::script dataScript(libbitcoin::system::machine::operation::list({libbitcoin::system::machine::opcode::return_, data}));
		tx.outputs().push_back(libbitcoin::system::chain::output(0, dataScript));

		libbitcoin::system::chain::script changeScript = libbitcoin::system::chain::script::to_pay_key_hash_pattern(address.hash()); // takes  ashort_hash which is a byte_array<20>
		libbitcoin::system::chain::output changeOutput(0, changeScript);
		tx.outputs().push_back(changeOutput);

		for (auto & category : categories) {
			libbitcoin::system::chain::script categoryScript = libbitcoin::system::chain::script::to_pay_key_hash_pattern(category);
			tx.outputs().push_back(libbitcoin::system::chain::output(0, categoryScript));
		}

		uint64_t funded = 0;

		libbitcoin::system::chain::script utxo_script = libbitcoin::system::chain::script::to_pay_key_hash_pattern(address.hash());
		libbitcoin::system::chain::point_value utxo = utxos.back(); // utxos could be empty, in which case segfault happens atm
		utxos.pop_back();
		funded += utxo.value();
		tx.inputs().push_back(libbitcoin::system::chain::input(utxo, utxo_script, 0xffffffff));

		uint64_t fee = fee_per_byte * tx.to_data().size() + fee_per_sigop * tx.signature_operations(true, true);
		tx.outputs()[1].set_value(funded - fee);

		for (auto & input : tx.inputs()) {
			libbitcoin::system::endorsement signature;
			if (!libbitcoin::system::chain::script::create_endorsement(signature, privkey.secret(), utxo_script, tx, 0, libbitcoin::system::machine::all)) {
				throw std::runtime_error("Failed to sign input to tx");
			}
			libbitcoin::system::chain::script unlocking_script(libbitcoin::system::machine::operation::list({
				libbitcoin::system::machine::operation(signature),
				libbitcoin::system::machine::operation(libbitcoin::system::to_chunk(privkey.to_public().point()))
			}));
			input.set_script(unlocking_script);
		}

		std::cout << "fees: " << tx.fees() << std::endl;
		std::cout << "fee: " << fee << std::endl;
		std::cout << "sigops: " << tx.signature_operations(true, true) << std::endl;
		auto raw = tx.to_data();
		std::cout << "bytes: " << raw.size() << std::endl;

		std::cout << "Transaction: " << libbitcoin::system::encode_base16(tx.to_data()) << std::endl;
		std::cout << "txid: " << libbitcoin::system::encode_hash(tx.hash()) << std::endl;
		//*/


		///*
		libbitcoin::system::code result = libbitcoinasync<libbitcoin::system::code>("validate", &Client::transaction_pool_validate2, tx);
		if (result)  {
			throw std::system_error(result, "validate");
		}
		//*/
		utxos.push_back(libbitcoin::system::chain::point_value(libbitcoin::system::chain::point(tx.hash(), 1), tx.outputs()[1].value()));
		libbitcoinasync<int>("broadcast", &Client::transaction_pool_broadcast, tx);
		return libbitcoin::system::encode_hash(tx.hash());
	}

private:
	using Client = libbitcoin::client::obelisk_client;
	/*
	template <typename... Result>
	using result_handler = std::function<void(libbitcoin::system::code const &, Result const &...)>;
	*/
	Client client;
	libbitcoin::system::wallet::ec_private privkey;
	libbitcoin::system::wallet::payment_address address;
	libbitcoin::system::hash_digest paymentkey;
	libbitcoin::system::chain::point_value::list utxos;

	void on_update(libbitcoin::system::code const & code, uint16_t sequence, size_t height, libbitcoin::system::hash_digest const & hash_digest)
	{
		std::cout << code << " " << sequence << " " << height << libbitcoin::system::encode_hash(hash_digest) << std::endl;
	}

private:
	template <typename Result, typename Function, typename... Args>
	Result libbitcoinasync(std::string information, Function function, Args... args)
	{
		libbitcoin::system::code error;
		Result result;
		(client.*function)([&](libbitcoin::system::code const & code, auto... args){
			error = code;
			result = Result(args...);
		}, args...);
		if (error) {
			throw std::system_error(error, information);
		}
		return result;
	}
};

int main()
{
	string test_string = "Have fun being able to retain something rather than having it destroyed!";
	vector<uint8_t> test_data(test_string.begin(), test_string.end());

	//std::string cryptkey = ashesofshreddeddocument::new_encrypted_user("Caring behavior is more precious than anything else.");
	std::string cryptkey = "6PYUziwRCpz5y2dpDVCdQ9y29X5Ag9UBrxXVpeG5q3RinbGFdHbnbw2YTd";
	std::cout << "Key: " << cryptkey << std::endl;
	std::string clearkey = ashesofshreddeddocument::decrypt_user(cryptkey, "Caring behavior is more precious than anything else.");
	ashesofshreddeddocument bonfireofincriminatingpaperwork(clearkey);
	bonfireofincriminatingpaperwork.process(test_data, {});
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
