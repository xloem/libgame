#include <Filecoin.hpp>
#include <jsonrpccpp/client/connectors/httpclient.h>

class lotus {
public:
  struct node_options {
    std::string url;
    std::string acct;
    std::string token;
  };
  static node_options default_options()
  {
    return {
      "https://api.node.glif.io/rpc/v0"
    };
  }

  struct upload_data {
    std::string filename
  }

  lotus(node_options const & options)
  : options(options),
    httpclient(options.url),
    filecoin(httpclient, jsonrpc::JSONRPC_CLIENT_V2)
  { }

  // find member function for getting price
  Json::Value queryAsk(std::string miner, std::string peer = "")
  {
    if (peer == "") {
      peer = filecoin.StateMinerInfo(miner, {Json::arrayValue})["PeerId"];
    }
    // Price
    // VerifiedPrice
    // MinPieceSize
    // MaxPieceSize
    // Miner
    // Timestamp
    // Expiry
    // SeqNo
    struct StorageAsk {
      uint64_t Expiry;
      uint64_t MaxPieceSize;
      uint64_t MinPieceSize;
      std::string Miner;
      double Price;
      uint64_t SeqNo;
      uint64_t Timestamp;
      double VerifiedPrice;
    };
    return filecoin.ClientQueryAsk(peer, miner);
  }

  std::string startDeal(std::string cid, std::string miner, double price, uint64_t blocksDuration, int64_t startEpoch = -1, bool fast = true, bool verified = false, double provCollateral = 0)
  {
      // a miner can be asked for their price
      // via `lotus client query-ask <miner address>`
    // piecesize when padded, should fit in sectorsize of miner
    // DealStartEpoch is when to start the deal, it can be <= 0 to automatically pick a duration from now (8 days, an approximation of SealDuration + PreCommit + MaxProveCommitDuration + buffer)
    // miner is passed to NewStorageProviderInfo
    // deal duration is from MinBlocksDuration, about 2 blocks an hour?
    // price and collateral are passed to SMDealClient.ProposeStorageDeal
    // collateral is a bigint

    Json::Value param;
      Json::Value data;
        Json::Value data_root;
          data_root["/"] = cid;
        data["Root"] = data_root;
    param["Data"] = data;
    param["Wallet"] = options.acct;
    param["Miner"] = miner;
    param["EpochPrice"] = price;
    param["MinBlocksDuration"] = blocksDuration;
    param["ProviderCollateral"] = provCollateral;
    param["DealStartEpoch"] = startEpoch;
    param["FastRetrieval"] = fast;
    param["VerifiedDeal"] = verified;

    return filecoin.ClientStartDeal(param);
  }

  std::string import(std::string path, bool isCAR)
  {
    return filecoin.ClientImport(path, isCAR)["Root"]["/"];
  }

private:
  node_options options;
  jsonrpc::HttpClient httpclient;
  lotus::Filecoin filecoin;
};
