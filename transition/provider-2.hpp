#pragma once

#include <chrono>
#include <span>
#include <string>
#include <vector>

namespace libgame {

class timeout_error : public std::runtime_error {
public:
  timeout_error(char const * what_arg, std::chrono::milliseconds timeout)
  : std::runtime_error(what_arg),
    timeout(timeout)
  { }

  std::chrono::milliseconds timeout;
};

// this could be upgraded to be a queue of spans
// boost has a sync_timed_queue that could be used internally, dunno
// it would be span-by-span and the finish() functions would be associated with each
// queued span, to reuse the underlying data.
class buffer_pipe {
public:
  buffer_pipe(uint64_t total_size = 0)
  : open(true)
  { }

  // send data.  an empty span closes the pipe
  void send(std::span<uint8_t> data) {
    consumed = {}
    produced.set_value(data);
  }
  // throw an exception on the receiver side
  void send_error(std::exception_ptr error) {
    produced.set_error(error);
  }
  // wait for receiver to finish with buffer
  void send_finish() {
    // for the case when is only one buffer that spans are from,
    // this doesn't go in send() so that the buffer can be refilled after consumed
    consumed.get_future().get_value();
    produced = {};
  }

  // returns empty data when closed
  std::span<uint8_t> const & recv(std::chrono::milliseconds timeout = 0) {
    std::span<uint8_t> result;
    if (open) {
      std::future<uint8_t> future = produced.get_future();

      if (timeout) {
        // wait for timeout, throw if reached
        if (std::future_status::timeout == future.wait_for(timeout)) {
          exception_ptr error = std::make_exception_ptr(timeout_error("buffer_pipe::recv timed out", timeout));
          recv_error(error);
          std::rethrow_exception(error);
        }
      }

      result = future.get_value();
      if (!result.size) {
        open = false;
      }
    }
    return result;
  }
  // throw an error in the sender
  void recv_error(std::exception_ptr error) {
    consumed.set_error(error);
  }
  // call when done with span, so sender can send another
  void recv_finish() {
    // letting the receiver call this before possible other processing gives
    // the sender more time to start using the buffer again earlier
    consumed.set_value();
  }

private:
  std::promise<std::span<uint8_t>> produced;
  std::promise<std::span<void>> consumed;
  bool open;
};

std::ostream & operator<<(std::ostream & output, buffer_pipe & input) {
  std::span<uint8_t> data;
  do {
    data = input.recv();
    output.write(data.begin(), data.size());
  } while(data.size());
  return output;
}

std::istream & operator>>(std::istream & input, buffer_pipe & output) {
  std::vector<uint8_t> data(4096);
  do {
    input.read(data.begin(), data.capacity();
    data.resize(input.gcount());
    output.send(data);
  } while (input.gcount() == data.capacity());
  return input;
}

class storage_interface {

  virtual std::vector<std::string> providers() const = 0;
  virtual std::vector<std::string> providers(std::string const & cid) const = 0;

  struct tx {
    enum kind {
      download;
      upload;
    } kind;
    std::string id;
    std::string provider;

    std::chrono::steady_clock::time_point begin;
    std::chrono::steady_clock::time_point eta;
    uint64_t transferred;
    uint64_t total;

    std::string cid;
    // completion can be waited for via the buffer_pipes here
    std::map<std::string, buffer_pipe> data;

    std::mutex mtx;
  };

  virtual tx & upload(
    std::string const & provider,
    std::vector<std::string> const & pathnames
  ) = 0;

  virtual tx & download(
    std::string const & provider,
    std::string const & cid,
    std::map<std::string, std::pair<uint64_t,uint64_t>> subranges = {}
  ) = 0;

  virtual std::map<std::string, uint64_t> query(
    std::string const & provider,
    std::string const & cid
  ) = 0;

  virtual tx & poll_tx(std::string transfer_id) = 0;
  virtual void cancel_tx(tx & transaction) = 0;
};

// from siaskynetpp
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <unordered_set>

class storage_siaskynet {
public:
  std::vector<std::string> providers() const override
  {
	  std::unordered_set<std::string> urls;
	  auto response = cpr::Get(cpr::Url{"https://siastats.info/dbs/skynet_current.json"});
	  std::string text;
	  if (response.error || response.status_code != 200) {
	  	// retrieved 2020-05-14
	  	text = R"([{"name":"SiaSky.net","files":[392823,7449,382702,5609],"size":[2.55,0.11,2.3,0.09],"link":"https://siasky.net","chartColor":"#666","version":"1.4.8-master","gitrevision":"a54efe103"},{"name":"SiaCDN.com","files":[659977],"size":[2.65],"link":"https://www.siacdn.com","chartColor":"#666","version":"1.4.8-master","gitrevision":"d47625aac"},{"name":"SkynetHub.io","files":[5408,0],"size":[0.04,0],"link":"https://skynethub.io","chartColor":"#666","version":"1.4.8-master","gitrevision":"ca21c97fc"},{"name":"SiaLoop.net","files":[40979],"size":[0.18],"link":"https://sialoop.net","chartColor":"#666","version":"1.4.7","gitrevision":"000eccb45"},{"name":"SkyDrain.net","files":[42242,914],"size":[0.23,0.04],"link":"https://skydrain.net","chartColor":"#666","version":"1.4.8","gitrevision":"1eb685ba8"},{"name":"Tutemwesi.com","files":[73918],"size":[0.32],"link":"https://skynet.tutemwesi.com","chartColor":"#666","version":"1.4.6-master","gitrevision":"c2a4d83"},{"name":"Luxor.tech","files":[21754],"size":[0.11],"link":"https://skynet.luxor.tech","chartColor":"#666","version":"1.4.5-master","gitrevision":"e1b995f"},{"name":"LightspeedHosting.com","files":[0],"size":[0],"link":"https://vault.lightspeedhosting.com","chartColor":"#666","version":"","gitrevision":""},{"name":"UTXO.no","files":[0],"size":[0],"link":"https://skynet.utxo.no","chartColor":"#666","version":"","gitrevision":""},{"name":"SkyPortal.xyz","files":[22858,0],"size":[0.14,0],"link":"https://skyportal.xyz","chartColor":"#666","version":"1.4.8","gitrevision":"1eb685ba8"}])";
R"([{"name":"SiaSky.net","files":[44343,159117,70931,1210933,141162,207609,97131,413804,462747,343233,604387,150182,91334,81409],"size":[0.52,1.64,0.51,7.84,1.51,1.58,1.08,34.14,29.49,37,11.59,8.65,9.51,8.26],"link":"https://siasky.net","chartColor":"#666","version":"1.6.0-master","gitrevision":"✗-e9e8a5b47"},{"name":"SkyDrain.net","files":[144269,3336],"size":[0.75,0.08],"link":"https://skydrain.net","chartColor":"#666","version":"1.5.0","gitrevision":"492f9b293"},{"name":"Tutemwesi.com","files":[78071],"size":[0.55],"link":"https://skynet.tutemwesi.com","chartColor":"#666","version":"1.4.6-master","gitrevision":"c2a4d83"},{"name":"SkyPortal.xyz","files":[429004],"size":[6.5],"link":"https://skyportal.xyz","chartColor":"#666","version":"1.5.6-master","gitrevision":"✗-7aa800095"}])";
	  } else {
	  	text = response.text;
	  }
	 	urls.insert("https://siasky.dev");
	 	urls.insert("https://siasky.net");
	  for (auto portal : nlohmann::json::parse(text)) {
	  	urls.insert(portal["link"]);
	  }
	  return {urls.begin(), urls.end()};
  }

  virtual std::vector<std::string> providers(std::string const & cid) const
  {
    return providers();
  }

  virtual tx & upload(
    std::string const & provider,
    std::vector<std::string> const & pathnames
  ) {
    // so here we would need to spawn a thread to read from the pathnames, i suppose.
    // it would make sense for the virtual functions to be the in-thread ones, at least eventually
    // they would just be passed a tx object
    //
    // transaction is indexed by its id.
    //      atm we have skynet and lotus.
    //      with skynet, we don't get a cid until the end
    //      but similarly, transfers are only valid for the lifetime of the network connection
    //      we'll probably want to find transfers that have to do with a cid, or a file.
    //      and from skynet, it's somewhat expected that the caller keep track of the cid
    //      the thread will definitely need a transfer object.  it seems okay for the thread
    //      to have the transfer object.  it would need to send the address back.
    //      we could have a thread introducing function that gives an address.
    std::
    std::thread(&storage_interface::upload_impl, transfer).detach();
    return transfer;
  }
  void upload_thread(std::promise<tx*> transfer_recipient)
  {
    tx transfer;
  }
  // who cleans up transactions when they are done?
  std::unordered_map<std::string, tx> txs;

  virtual tx & download(
    std::string const & provider,
    std::string const & cid,
    std::map<std::string, std::pair<uint64_t,uint64_t>> subranges = {}
  ) = 0;

  virtual std::map<std::string, uint64_t> query(
    std::string const & provider,
    std::string const & cid
  ) = 0;

  virtual tx & poll_tx(std::string transfer_id) = 0;
  virtual void cancel_tx(tx & transaction) = 0;

private:
  std::unordered_map<std::string, cpr::Session> provider_sessions;
};
