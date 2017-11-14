#pragma once

#include "core/Trader_Handler.h"

void on_book(CThostFtdcDepthMarketDataField *pDepthMarketData);

void on_response(CThostFtdcTradeField *pTrade);