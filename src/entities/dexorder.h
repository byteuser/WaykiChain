// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_DEX_ORDER_H
#define ENTITIES_DEX_ORDER_H

#include <string>

#include "id.h"
#include "asset.h"
#include "commons/types.h"
#include "commons/json/json_spirit.h"

typedef uint32_t DexID;

static const DexID DEX_RESERVED_ID = 0;

enum OrderSide: uint8_t {
    ORDER_BUY  = 1,
    ORDER_SELL = 2,
};

static const EnumTypeMap<OrderSide, string> ORDER_SIDE_NAMES = {
    {ORDER_BUY, "BUY"}, {ORDER_SELL, "SELL"}
};

inline bool CheckOrderSide(OrderSide orderSide) {
    return ORDER_SIDE_NAMES.find(orderSide) != ORDER_SIDE_NAMES.end();
}

inline const std::string &GetOrderSideName(OrderSide orderSide) {
    auto it = ORDER_SIDE_NAMES.find(orderSide);
    if (it != ORDER_SIDE_NAMES.end())
        return it->second;
    assert(false && "not support unknown OrderSide");
    return EMPTY_STRING;
}

enum OrderType: uint8_t {
    ORDER_LIMIT_PRICE   = 1, //!< limit price order type
    ORDER_MARKET_PRICE  = 2  //!< market price order type
};

static const EnumTypeMap<OrderType, string> ORDER_TYPE_NAMES = {
    {ORDER_LIMIT_PRICE, "LIMIT_PRICE"}, {ORDER_MARKET_PRICE, "MARKET_PRICE"}
};

inline bool CheckOrderType(OrderType orderType) {
    return ORDER_TYPE_NAMES.find(orderType) != ORDER_TYPE_NAMES.end();
}

inline const std::string &GetOrderTypeName(OrderType orderType) {
    auto it = ORDER_TYPE_NAMES.find(orderType);
    if (it != ORDER_TYPE_NAMES.end())
        return it->second;
    assert(false && "not support unknown OrderType");
    return EMPTY_STRING;
}

enum OrderGenerateType: uint8_t {
    EMPTY_ORDER         = 0,
    USER_GEN_ORDER      = 1,
    SYSTEM_GEN_ORDER    = 2
};

static const EnumTypeMap<OrderGenerateType, string> ORDER_GEN_TYPE_NAMES = {
    {EMPTY_ORDER, "EMPTY_ORDER"},
    {USER_GEN_ORDER, "USER_GEN_ORDER"},
    {SYSTEM_GEN_ORDER, "SYSTEM_GEN_ORDER"}
};

inline const string &GetOrderGenTypeName(OrderGenerateType genType) {
    auto it = ORDER_GEN_TYPE_NAMES.find(genType);
    if (it != ORDER_GEN_TYPE_NAMES.end())
        return it->second;
    return EMPTY_STRING;
}

 struct OrderOperatorMode {
    enum Mode: uint8_t {
        DEFAULT,       // simple mode
        REQUIRE_AUTH       // require dex operator authenticate (should have operator signature in tx)
    };
    Mode value = DEFAULT;

    OrderOperatorMode() {}
    OrderOperatorMode(Mode valueIn): value(valueIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&)value);
    )

    bool IsValid();

    const string& Name() const;

    static bool Parse(const string &name, OrderOperatorMode &mode);

    static OrderOperatorMode GetDefault();

    bool operator==(const Mode &modeValue) const { return this->value == modeValue; }
    bool operator!=(const Mode &modeValue) const { return this->value != modeValue; }

private:
    static const EnumTypeMap<Mode, string> VALUE_NAME_MAP;
    static const unordered_map<string, Mode> NAME_VALUE_MAP;
};

struct CDEXOrderDetail {
    OrderOperatorMode mode;
    DexID dex_id = 0;
    uint64_t operator_fee_ratio        = 0;                 //!< operator fee ratio, effective in REQUIRE_AUTH mode
    OrderGenerateType generate_type    = EMPTY_ORDER;       //!< generate type
    OrderType order_type               = ORDER_LIMIT_PRICE; //!< order type
    OrderSide order_side               = ORDER_BUY;         //!< order side
    TokenSymbol coin_symbol            = "";                //!< coin symbol
    TokenSymbol asset_symbol           = "";                //!< asset symbol
    uint64_t coin_amount               = 0;                 //!< amount of coin to buy/sell asset
    uint64_t asset_amount              = 0;                 //!< amount of asset to buy/sell
    uint64_t price                     = 0;                 //!< price in coinType want to buy/sell asset
    CTxCord  tx_cord                   = CTxCord();         //!< related tx cord
    CRegID user_regid                  = CRegID();          //!< user regid
    uint64_t total_deal_coin_amount    = 0;                 //!< total deal coin amount
    uint64_t total_deal_asset_amount   = 0;                 //!< total deal asset amount

public:
    static shared_ptr<CDEXOrderDetail> CreateUserBuyLimitOrder(
        const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol, const uint64_t assetAmountIn,
        const uint64_t priceIn, const CTxCord &txCord, const CRegID &userRegid);

public:
    IMPLEMENT_SERIALIZE(
        READWRITE(mode);
        READWRITE(VARINT(dex_id));
        READWRITE(VARINT(operator_fee_ratio));
        READWRITE((uint8_t&)generate_type);
        READWRITE((uint8_t&)order_type);
        READWRITE((uint8_t&)order_side);
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(coin_amount));
        READWRITE(VARINT(asset_amount));
        READWRITE(VARINT(price));
        READWRITE(tx_cord);
        READWRITE(user_regid);
        READWRITE(VARINT(total_deal_coin_amount));
        READWRITE(VARINT(total_deal_asset_amount));

        READWRITE(tx_cord);
    )

    bool IsEmpty() const {
        return generate_type == EMPTY_ORDER;
    }
    void SetEmpty() {
        generate_type             = EMPTY_ORDER;
        order_type                = ORDER_LIMIT_PRICE;
        order_side                = ORDER_BUY;
        coin_symbol               = "";
        asset_symbol              = "";
        coin_amount               = 0;
        asset_amount              = 0;
        price                     = 0;
        tx_cord.SetEmpty();
        user_regid.SetEmpty();
        total_deal_coin_amount    = 0;
        total_deal_asset_amount   = 0;
    }

    string ToString() const;
    void ToJson(json_spirit::Object &obj) const;
};


// for all active order db: orderId -> CDEXActiveOrder
struct CDEXActiveOrder {
    OrderGenerateType generate_type     = EMPTY_ORDER;  //!< generate type
    CTxCord  tx_cord                   = CTxCord();    //!< related tx cord
    uint64_t total_deal_coin_amount    = 0;            //!< total deal coin amount
    uint64_t total_deal_asset_amount   = 0;            //!< total deal asset amount

    CDEXActiveOrder() {}

    CDEXActiveOrder(OrderGenerateType generateType, const CTxCord &txCord):
        generate_type(generateType), tx_cord(txCord)
    {}

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&)generate_type);
        READWRITE(tx_cord);
        READWRITE(VARINT(total_deal_coin_amount));
        READWRITE(VARINT(total_deal_asset_amount));
    )

    bool IsEmpty() const {
        return generate_type == EMPTY_ORDER;
    }
    void SetEmpty() {
        generate_type  = EMPTY_ORDER;
        total_deal_coin_amount    = 0;
        total_deal_asset_amount   = 0;
        tx_cord.SetEmpty();
    }
};

// order txid -> sys order data
// order txid:
//   (1) CCDPStakeTx, create sys buy market order for WGRT by WUSD when alter CDP and the interest is WUSD
//   (2) CCDPRedeemTx, create sys buy market order for WGRT by WUSD when the interest is WUSD
//   (3) CCDPLiquidateTx, create sys buy market order for WGRT by WUSD when the penalty is WUSD

// txid -> sys order data
class CDEXSysOrder {
public:// create functions
    static shared_ptr<CDEXOrderDetail> CreateBuyMarketOrder(const CTxCord &txCord, const TokenSymbol &coinSymbol,
        const TokenSymbol &assetSymbol, uint64_t coinAmountIn);

    static shared_ptr<CDEXOrderDetail> CreateSellMarketOrder(const CTxCord &txCord, const TokenSymbol &coinSymbol,
        const TokenSymbol &assetSymbol, uint64_t assetAmountIn);

    static shared_ptr<CDEXOrderDetail> Create(OrderType orderType, OrderSide orderSide, const CTxCord &txCord,
        const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol, uint64_t coiAmountIn, uint64_t assetAmountIn);
};

// dex operator
struct DexOperatorDetail {
    CRegID owner_regid;                   // owner uid of exchange
    CRegID match_regid;                   // match uid
    string name              = "";       // domain name
    string portal_url        = "";
    uint64_t maker_fee_ratio = 0;
    uint64_t taker_fee_ratio = 0;
    string memo              = "";
    // TODO: state

    IMPLEMENT_SERIALIZE(
        READWRITE(owner_regid);
        READWRITE(match_regid);
        READWRITE(name);
        READWRITE(portal_url);
        READWRITE(VARINT(maker_fee_ratio));
        READWRITE(VARINT(taker_fee_ratio));
        READWRITE(memo);
    )

    bool IsEmpty() const {
        return owner_regid.IsEmpty() && match_regid.IsEmpty() && name.empty() && portal_url.empty() &&
               maker_fee_ratio == 0 && taker_fee_ratio == 0 && memo.empty();
    }

    void SetEmpty() {
        owner_regid.SetEmpty(); match_regid.SetEmpty(); name = ""; portal_url = ""; maker_fee_ratio = 0;
        taker_fee_ratio = 0; memo = "";
    }
};

#endif //ENTITIES_DEX_ORDER_H
