// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_DEX_H
#define TX_DEX_H

#include "entities/asset.h"
#include "entities/dexorder.h"
#include "tx.h"
#include "persistence/dexdb.h"
#include <optional>

class CDEXOrderBaseTx : public CBaseTx {
public:
    // enum OrderOperatorMode: uint8_t {
    //     OrderOperatorMode::DEFAULT,       // simple mode
    //     OPERATOR_AUTH       // require dex operator authenticate (should have operator signature in tx)
    // };
public:
    OrderOperatorMode mode        = OrderOperatorMode::DEFAULT;
    DexID dex_id                = 0;                 //!< dex id
    uint64_t operator_fee_ratio = 0;                 //!< order fee ratio, effective in OPERATOR_AUTH mode, 0 in DEFAULT mode
    OrderType order_type        = ORDER_LIMIT_PRICE; //!< order type
    OrderSide order_side        = ORDER_BUY;         //!< order side
    TokenSymbol coin_symbol     = "";                //!< coin symbol
    TokenSymbol asset_symbol    = "";                //!< asset symbol
    uint64_t coin_amount        = 0;                 //!< amount of coin to buy/sell asset
    uint64_t asset_amount       = 0;                 //!< amount of asset to buy/sell
    uint64_t price              = 0;                 //!< price in coinType want to buy/sell asset
    string memo                 = "";                //!< memo
    std::optional<CSignaturePair> operator_signature_pair;

    using CBaseTx::CBaseTx;
public:
    void SetOperatorRegid(const CRegID *pOperatorRegidIn) {
        if (pOperatorRegidIn != nullptr) {
            operator_signature_pair = make_optional<CSignaturePair>(*pOperatorRegidIn);
        }
    }

    std::optional<CRegID> GetOperatorRegid() const {
        std::optional<CRegID> ret;
        if (operator_signature_pair)
            ret = make_optional<CRegID>(operator_signature_pair.value().regid);
        return ret;
    }

    bool CheckOrderAmountRange(CValidationState &state, const string &title,
                             const TokenSymbol &symbol, const int64_t amount);

    bool CheckOrderPriceRange(CValidationState &state, const string &title,
                              const TokenSymbol &coin_symbol, const TokenSymbol &asset_symbol,
                              const int64_t price);
    bool CheckOrderSymbols(CValidationState &state, const string &title,
                           const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol);

    bool CheckDexOperatorExist(CTxExecuteContext &context);

    bool CheckOrderFeeRate(CTxExecuteContext &context, const string &title);
    bool CheckOrderOperator(CTxExecuteContext &context, const string &title);

    bool ProcessOrder(CTxExecuteContext &context, CAccount &txAccount, const string &title);
    bool FreezeBalance(CTxExecuteContext &context, CAccount &account,
                       const TokenSymbol &tokenSymbol, const uint64_t &amount, const string &title);

public:
    static uint64_t CalcCoinAmount(uint64_t assetAmount, const uint64_t price);

};

////////////////////////////////////////////////////////////////////////////////
// buy limit order tx
class CDEXBuyLimitOrderBaseTx : public CDEXOrderBaseTx {
public:
    CDEXBuyLimitOrderBaseTx(TxType nTxTypeIn) : CDEXOrderBaseTx(nTxTypeIn) {}

    CDEXBuyLimitOrderBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn,
                            const TokenSymbol &feeSymbol, uint64_t fees, OrderOperatorMode modeIn,
                            DexID dexIdIn, uint64_t orderFeeRatioIn, const TokenSymbol &coinSymbol,
                            const TokenSymbol &assetSymbol, uint64_t assetAmountIn,
                            uint64_t priceIn, const string &memoIn, const CRegID *pOperatorRegidIn)
        : CDEXOrderBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees) {
        mode            = modeIn;
        dex_id          = dexIdIn;
        operator_fee_ratio = orderFeeRatioIn;
        order_type      = ORDER_LIMIT_PRICE;
        order_side      = ORDER_BUY;
        coin_symbol     = coinSymbol;
        asset_symbol    = assetSymbol;
        coin_amount     = 0; // default 0 in buy limit order
        asset_amount    = assetAmountIn;
        price           = priceIn;
        memo            = memoIn;
        SetOperatorRegid(pOperatorRegidIn);
    }

    string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

class CDEXBuyLimitOrderTx : public CDEXBuyLimitOrderBaseTx {

public:
    CDEXBuyLimitOrderTx() : CDEXBuyLimitOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX) {}

    CDEXBuyLimitOrderTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                        uint64_t fees, const TokenSymbol &coinSymbol,
                        const TokenSymbol &assetSymbol, uint64_t assetAmountIn, uint64_t priceIn)
        : CDEXBuyLimitOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees,
                                  OrderOperatorMode::DEFAULT, DEX_RESERVED_ID, 0, coinSymbol,
                                  assetSymbol, assetAmountIn, priceIn, "", nullptr) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));
        READWRITE(VARINT(price));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << coin_symbol << asset_symbol
               << VARINT(asset_amount) << VARINT(price);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXBuyLimitOrderTx>(*this); }

    virtual bool CheckTx(CTxExecuteContext &context);
};


class CDEXBuyLimitOrderExTx : public CDEXBuyLimitOrderBaseTx {
public:
    CDEXBuyLimitOrderExTx() : CDEXBuyLimitOrderBaseTx(DEX_LIMIT_BUY_ORDER_EX_TX) {}

    CDEXBuyLimitOrderExTx(const CUserID &txUidIn, int32_t validHeightIn,
                          const TokenSymbol &feeSymbol, uint64_t fees, OrderOperatorMode modeIn,
                          DexID dexIdIn, uint64_t orderFeeRatioIn, const TokenSymbol &coinSymbol,
                          const TokenSymbol &assetSymbol, uint64_t assetAmountIn, uint64_t priceIn,
                          const string &memoIn, const CRegID *pOperatorRegidIn)
        : CDEXBuyLimitOrderBaseTx(DEX_LIMIT_BUY_ORDER_EX_TX, txUidIn, validHeightIn, feeSymbol,
                                  fees, modeIn, dexIdIn, orderFeeRatioIn, coinSymbol, assetSymbol,
                                  assetAmountIn, priceIn, memoIn, pOperatorRegidIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(VARINT((uint8_t&)mode));
        READWRITE(VARINT(dex_id));
        READWRITE(VARINT(operator_fee_ratio));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));
        READWRITE(VARINT(price));
        READWRITE(memo);

        READWRITE(operator_signature_pair);
        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << VARINT((uint8_t&)mode) << VARINT(dex_id)
               << VARINT(operator_fee_ratio) << coin_symbol << asset_symbol << VARINT(asset_amount)
               << VARINT(price) << memo << GetOperatorRegid();
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXBuyLimitOrderExTx>(*this); }

    virtual bool CheckTx(CTxExecuteContext &context);
};

////////////////////////////////////////////////////////////////////////////////
// sell limit order tx
class CDEXSellLimitOrderBaseTx : public CDEXOrderBaseTx {

public:
    CDEXSellLimitOrderBaseTx(TxType nTxTypeIn) : CDEXOrderBaseTx(nTxTypeIn) {}

    CDEXSellLimitOrderBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn,
                             const TokenSymbol &feeSymbol, uint64_t fees, OrderOperatorMode modeIn,
                             DexID dexIdIn, uint64_t orderFeeRatioIn, TokenSymbol coinSymbol,
                             const TokenSymbol &assetSymbol, uint64_t assetAmountIn,
                             uint64_t priceIn, const string &memoIn,
                             const CRegID *pOperatorRegidIn)
        : CDEXOrderBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees) {
        mode            = modeIn;
        dex_id          = dexIdIn;
        operator_fee_ratio = orderFeeRatioIn;
        order_type      = ORDER_LIMIT_PRICE;
        order_side      = ORDER_SELL;
        coin_symbol     = coinSymbol;
        asset_symbol    = assetSymbol;
        coin_amount     = 0; // default 0 in sell limit order
        asset_amount    = assetAmountIn;
        price           = priceIn;
        memo            = memoIn;
        SetOperatorRegid(pOperatorRegidIn);
    }

    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

class CDEXSellLimitOrderTx : public CDEXSellLimitOrderBaseTx {

public:
    CDEXSellLimitOrderTx() : CDEXSellLimitOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX) {}

    CDEXSellLimitOrderTx(const CUserID &txUidIn, int32_t validHeightIn,
                         const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                         const TokenSymbol &assetSymbol, uint64_t assetAmount, uint64_t priceIn)
        : CDEXSellLimitOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees,
                                   OrderOperatorMode::DEFAULT, DEX_RESERVED_ID, 0, coinSymbol,
                                   assetSymbol, assetAmount, priceIn, "", nullptr) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));
        READWRITE(VARINT(price));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << coin_symbol << asset_symbol << VARINT(asset_amount) << VARINT(price);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSellLimitOrderTx>(*this); }
    // TODO: need check tx?
    //virtual bool CheckTx(CTxExecuteContext &context);
};

class CDEXSellLimitOrderExTx : public CDEXSellLimitOrderBaseTx {

public:
    CDEXSellLimitOrderExTx() : CDEXSellLimitOrderBaseTx(DEX_LIMIT_SELL_ORDER_EX_TX) {}

    CDEXSellLimitOrderExTx(const CUserID &txUidIn, int32_t validHeightIn,
                           const TokenSymbol &feeSymbol, uint64_t fees, OrderOperatorMode modeIn,
                           DexID dexIdIn, uint64_t orderFeeRatioIn, TokenSymbol coinSymbol,
                           const TokenSymbol &assetSymbol, uint64_t assetAmount, uint64_t priceIn,
                           const string &memoIn, const CRegID *pOperatorRegidIn)
        : CDEXSellLimitOrderBaseTx(DEX_LIMIT_SELL_ORDER_EX_TX, txUidIn, validHeightIn, feeSymbol,
                                   fees, modeIn, dexIdIn, orderFeeRatioIn, coinSymbol, assetSymbol,
                                   assetAmount, priceIn, memoIn, pOperatorRegidIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(VARINT((uint8_t&)mode));
        READWRITE(VARINT(dex_id));
        READWRITE(VARINT(operator_fee_ratio));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));
        READWRITE(VARINT(price));
        READWRITE(memo);

        READWRITE(operator_signature_pair);
        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << VARINT((uint8_t&)mode) << VARINT(dex_id)
               << VARINT(operator_fee_ratio) << coin_symbol << asset_symbol << VARINT(asset_amount)
               << VARINT(price) << memo << GetOperatorRegid();
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSellLimitOrderExTx>(*this); }
    // TODO: need check tx?
    //virtual bool CheckTx(CTxExecuteContext &context);
};

////////////////////////////////////////////////////////////////////////////////
// buy market order tx
class CDEXBuyMarketOrderBaseTx : public CDEXOrderBaseTx {
public:
    CDEXBuyMarketOrderBaseTx(TxType nTxTypeIn) : CDEXOrderBaseTx(nTxTypeIn) {}

    CDEXBuyMarketOrderBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn,
                             const TokenSymbol &feeSymbol, uint64_t fees, OrderOperatorMode modeIn,
                             DexID dexIdIn, uint64_t orderFeeRatioIn, TokenSymbol coinSymbol,
                             const TokenSymbol &assetSymbol, uint64_t coinAmountIn,
                             const string &memoIn, const CRegID *pOperatorRegidIn)
        : CDEXOrderBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees) {
        mode            = modeIn;
        dex_id          = dexIdIn;
        operator_fee_ratio = orderFeeRatioIn;
        order_type      = ORDER_MARKET_PRICE;
        order_side      = ORDER_BUY;
        coin_symbol     = coinSymbol;
        asset_symbol    = assetSymbol;
        coin_amount     = coinAmountIn;
        asset_amount    = 0; // default 0 in buy market order
        price           = 0; // default 0 in buy market order
        memo            = memoIn;
        SetOperatorRegid(pOperatorRegidIn);
    }

    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
// private:
//     TokenSymbol coin_symbol;   //!< coin type (wusd) to buy asset
//     TokenSymbol asset_symbol;  //!< asset type
//     uint64_t coin_amount;      //!< amount of target coin to spend for buying asset
};

class CDEXBuyMarketOrderTx : public CDEXBuyMarketOrderBaseTx {
public:
    CDEXBuyMarketOrderTx() : CDEXBuyMarketOrderBaseTx(DEX_MARKET_BUY_ORDER_TX) {}

    CDEXBuyMarketOrderTx(const CUserID &txUidIn, int32_t validHeightIn,
                         const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                         const TokenSymbol &assetSymbol, uint64_t coinAmountIn)
        : CDEXBuyMarketOrderBaseTx(DEX_MARKET_BUY_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees,
                                   OrderOperatorMode::DEFAULT, DEX_RESERVED_ID, 0, coinSymbol,
                                   assetSymbol, coinAmountIn, "", nullptr) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(coin_amount));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << coin_symbol << asset_symbol
               << VARINT(coin_amount);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXBuyMarketOrderTx>(*this); }

    // TODO: need check tx??
    //virtual bool CheckTx(CTxExecuteContext &context);
};


class CDEXBuyMarketOrderExTx : public CDEXBuyMarketOrderBaseTx {
public:
    CDEXBuyMarketOrderExTx() : CDEXBuyMarketOrderBaseTx(DEX_MARKET_BUY_ORDER_EX_TX) {}

    CDEXBuyMarketOrderExTx(const CUserID &txUidIn, int32_t validHeightIn,
                           const TokenSymbol &feeSymbol, uint64_t fees, OrderOperatorMode modeIn,
                           DexID dexIdIn, uint64_t orderFeeRatioIn, TokenSymbol coinSymbol,
                           const TokenSymbol &assetSymbol, uint64_t coinAmountIn,
                           const string &memoIn, const CRegID *pOperatorRegidIn)
        : CDEXBuyMarketOrderBaseTx(DEX_MARKET_BUY_ORDER_EX_TX, txUidIn, validHeightIn, feeSymbol,
                                   fees, modeIn, dexIdIn, orderFeeRatioIn, coinSymbol, assetSymbol,
                                   coinAmountIn, memo, pOperatorRegidIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(VARINT((uint8_t&)mode));
        READWRITE(VARINT(dex_id));
        READWRITE(VARINT(operator_fee_ratio));
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(coin_amount));
        READWRITE(memo);

        READWRITE(operator_signature_pair);
        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << VARINT((uint8_t&)mode) << VARINT(dex_id)
               << VARINT(operator_fee_ratio) << coin_symbol << asset_symbol << VARINT(coin_amount)
               << memo << GetOperatorRegid();
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXBuyMarketOrderExTx>(*this); }

    // TODO: need check tx??
    //virtual bool CheckTx(CTxExecuteContext &context);
};

////////////////////////////////////////////////////////////////////////////////
// sell market order tx
class CDEXSellMarketOrderBaseTx : public CDEXOrderBaseTx {
public:
    CDEXSellMarketOrderBaseTx(TxType nTxTypeIn) : CDEXOrderBaseTx(nTxTypeIn) {}

    CDEXSellMarketOrderBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn,
                              const TokenSymbol &feeSymbol, uint64_t fees, OrderOperatorMode modeIn,
                              DexID dexIdIn, uint64_t orderFeeRatioIn, TokenSymbol coinSymbol,
                              const TokenSymbol &assetSymbol, uint64_t assetAmountIn,
                              const string &memoIn, const CRegID *pOperatorRegidIn)
        : CDEXOrderBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees) {
        mode            = modeIn;
        dex_id          = dexIdIn;
        operator_fee_ratio = orderFeeRatioIn;
        order_type      = ORDER_MARKET_PRICE;
        order_side      = ORDER_SELL;
        coin_symbol     = coinSymbol;
        asset_symbol    = assetSymbol;
        coin_amount     = 0; // default 0 in sell market order
        asset_amount    = assetAmountIn;
        price           = 0; // default 0 in sell market order
        memo            = memoIn;
        SetOperatorRegid(pOperatorRegidIn);
    }

    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

class CDEXSellMarketOrderTx : public CDEXSellMarketOrderBaseTx {
public:
    CDEXSellMarketOrderTx() : CDEXSellMarketOrderBaseTx(DEX_MARKET_SELL_ORDER_TX) {}

    CDEXSellMarketOrderTx(const CUserID &txUidIn, int32_t validHeightIn,
                          const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                          const TokenSymbol &assetSymbol, uint64_t assetAmountIn)
        : CDEXSellMarketOrderBaseTx(DEX_MARKET_SELL_ORDER_TX, txUidIn, validHeightIn, feeSymbol,
                                    fees, OrderOperatorMode::DEFAULT, DEX_RESERVED_ID, 0, coinSymbol,
                                    assetSymbol, assetAmountIn, "", nullptr) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << coin_symbol << asset_symbol << VARINT(asset_amount);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSellMarketOrderTx>(*this); }

    // TODO: need check tx??
    //virtual bool CheckTx(CTxExecuteContext &context);
};

class CDEXSellMarketOrderExTx : public CDEXSellMarketOrderBaseTx {
public:
    CDEXSellMarketOrderExTx() : CDEXSellMarketOrderBaseTx(DEX_MARKET_SELL_ORDER_EX_TX) {}

    CDEXSellMarketOrderExTx(const CUserID &txUidIn, int32_t validHeightIn,
                            const TokenSymbol &feeSymbol, uint64_t fees, OrderOperatorMode modeIn,
                            DexID dexIdIn, uint64_t orderFeeRatioIn, TokenSymbol coinSymbol,
                            const TokenSymbol &assetSymbol, uint64_t assetAmountIn,
                            const string &memoIn, const CRegID *pOperatorRegidIn)
        : CDEXSellMarketOrderBaseTx(DEX_MARKET_SELL_ORDER_EX_TX, txUidIn, validHeightIn, feeSymbol,
                                    fees, modeIn, dexIdIn, orderFeeRatioIn, coinSymbol, assetSymbol,
                                    assetAmountIn, memoIn, pOperatorRegidIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(VARINT((uint8_t&)mode));
        READWRITE(VARINT(dex_id));
        READWRITE(VARINT(operator_fee_ratio));
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));
        READWRITE(memo);

        READWRITE(operator_signature_pair);
        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << VARINT((uint8_t&)mode) << VARINT(dex_id)
               << VARINT(operator_fee_ratio) << coin_symbol << asset_symbol << VARINT(asset_amount)
               << memo << GetOperatorRegid();
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSellMarketOrderExTx>(*this); }
    // TODO: need check tx??
    //virtual bool CheckTx(CTxExecuteContext &context);
};

////////////////////////////////////////////////////////////////////////////////
// cancel order tx
class CDEXCancelOrderTx : public CBaseTx {
public:
    CDEXCancelOrderTx() : CBaseTx(DEX_CANCEL_ORDER_TX) {}

    CDEXCancelOrderTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                      uint64_t fees, uint256 orderIdIn)
        : CBaseTx(DEX_CANCEL_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees),
          orderId(orderIdIn) {}

    ~CDEXCancelOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(orderId);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << orderId;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXCancelOrderTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
public:
    uint256  orderId;       //!< id of oder need to be canceled.
};

////////////////////////////////////////////////////////////////////////////////
// settle order tx
struct DEXDealItem  {
    uint256 buyOrderId;
    uint256 sellOrderId;
    uint64_t dealPrice;
    uint64_t dealCoinAmount;
    uint64_t dealAssetAmount;

    IMPLEMENT_SERIALIZE(
        READWRITE(buyOrderId);
        READWRITE(sellOrderId);
        READWRITE(VARINT(dealPrice));
        READWRITE(VARINT(dealCoinAmount));
        READWRITE(VARINT(dealAssetAmount));
    )

    string ToString() const;
};

class CDEXSettleBaseTx: public CBaseTx {

public:
    CDEXSettleBaseTx(TxType nTxTypeIn) : CBaseTx(nTxTypeIn) {}

    CDEXSettleBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn,
                     const TokenSymbol &feeSymbol, uint64_t fees, DexID dexIdIn,
                     const vector<DEXDealItem> &dealItemsIn, const string &memoIn)
        : CBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees), dex_id(dexIdIn),
          dealItems(dealItemsIn), memo(memoIn) {}

    void AddDealItem(const DEXDealItem& item) {
        dealItems.push_back(item);
    }

    vector<DEXDealItem>& GetDealItems() { return dealItems; }

    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

protected:
    bool GetDealOrder(CCacheWrapper &cw, CValidationState &state, uint32_t index, const uint256 &orderId,
        const OrderSide orderSide, CDEXOrderDetail &dealOrder);
    bool CheckDexId(CTxExecuteContext &context, uint32_t i, uint32_t buyDexId, uint32_t sellDexId);

    OrderSide GetTakerOrderSide(const CDEXOrderDetail &buyOrder, const CDEXOrderDetail &sellOrder);
    uint64_t GetOperatorFeeRatio(const CDEXOrderDetail &order,
                                 const DexOperatorDetail &operatorDetail,
                                 const OrderSide &takerSide);
    bool CalcOrderFee(CTxExecuteContext &context, uint32_t i, uint64_t amount, uint64_t fee_ratio,
                      uint64_t &orderFee);

protected:
    DexID   dex_id;
    vector<DEXDealItem> dealItems;
    string memo;
};

class CDEXSettleTx: public CDEXSettleBaseTx {

public:
    CDEXSettleTx() : CDEXSettleBaseTx(DEX_TRADE_SETTLE_TX) {}

    CDEXSettleTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                 uint64_t fees, const vector<DEXDealItem> &dealItemsIn)
        : CDEXSettleBaseTx(DEX_TRADE_SETTLE_TX, txUidIn, validHeightIn, feeSymbol, fees,
                           DEX_RESERVED_ID, dealItemsIn, "") {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(dealItems);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << dealItems;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSettleTx>(*this); }

    // TODO: check tx
    //virtual bool CheckTx(CTxExecuteContext &context);
};

class CDEXSettleExTx: public CDEXSettleBaseTx {

public:
    CDEXSettleExTx() : CDEXSettleBaseTx(DEX_TRADE_SETTLE_TX) {}

    CDEXSettleExTx(const CUserID &txUidIn, int32_t validHeightIn,
                     const TokenSymbol &feeSymbol, uint64_t fees, DexID dexIdIn,
                     const vector<DEXDealItem> &dealItemsIn, const string &memoIn)
        : CDEXSettleBaseTx(DEX_TRADE_SETTLE_TX, txUidIn, validHeightIn, feeSymbol, fees, dexIdIn,
                           dealItemsIn, memoIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(VARINT(dex_id));
        READWRITE(dealItems);
        READWRITE(memo);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << dealItems;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSettleExTx>(*this); }

    // TODO: check tx
    //virtual bool CheckTx(CTxExecuteContext &context);
};

#endif  // TX_DEX_H