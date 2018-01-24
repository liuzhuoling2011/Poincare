//bollingerBands: {[k;n;data]      movingAvg: mavg[n;data];      md: sqrt mavg[n;data*data]-movingAvg*movingAvg;      movingAvg+/:(k*-1 0 1)*\:md};
//
////f:{x%y*6f*2204.6226};
//
//f:{(x*4.2)%y};
////update (Date - 1000000000*60*60*24) from `quote where Date.minute > 20:00:00 or Date.minute within 00:00:00 01:01:00;
//quoteData:quoteData,update PairAsk:f[LegTwoAsk1;LegOneBid1],PairBid:f[LegTwoBid1;LegOneAsk1] from quote;
//strategyData:-201#delete date,second from select by Date.date, 1 xbar Date.second from quoteData;
//delete from `strategyData where Date.minute within  00:00:00 09:30:00;
//delete from `strategyData where Date.minute within  11:30:00 13:00:00;
//delete from `strategyData where Date.minute within  15:00:00 23:00:00;
////delete from `strategyData where Date.minute within  15:00:00 21:00:05;
//update HigherBand2:bollingerBands[0.5;200;PairAsk][2],LowerBand2:bollingerBands[0.5;200;PairBid][0]  from `strategyData;
////delete from `strategyData where HigherBand2<1f or LowerBand2<1f;
//Signal: strategyData;
////update Signal:`Long from `Signal where PairAsk < LowerBand2;
////update Signal:`Short from `Signal where PairBid > HigherBand2; 
//update Signal:1i from `Signal where PairAsk < LowerBand2;
//update Signal:-1i from `Signal where PairBid > HigherBand2; 
//Signal2:select from Signal where Date = last Date;
////Signal2:select from Signal2 where ((Signal = `Long) or  (Signal = `Short));
//Signal2:select from Signal2 where ((Signal = 1) or  (Signal = -1));
//FinalSignal2:FinalSignal2,Signal2;
////delete from `FinalSignal2 where Date.minute within 08:30:00 09:30:05;
////delete from `FinalSignal2 where Date.minute within 11:30:00 13:00:05;
////delete from `FinalSignal2 where Date.minute within 13:00:00 13:00:05;
////delete from `FinalSignal2 where Date.minute within 15:00:00 16:00:05;
//ShortLong2:select from FinalSignal2  where (Signal<>(prev Signal));
//res:([]len:enlist count ShortLong2; b:-1#ShortLong2`LegTwoBid1; a:-1#ShortLong2`LegTwoAsk1;d:-1#ShortLong2`Signal);
//FinalSignal2:update LowerBand2:1.0, HigherBand2:1.0 from ShortLong2;
//
//
//
//cal:{ 
//    tempShortLong: ShortLong2;
//    tempShortLong:update Profit1: (((prev LegTwoBid1) - (LegTwoAsk1))) from tempShortLong; 
//    LongProfit: select  from tempShortLong where Signal = 1;
//    tempShortLong:update Profit1: (((LegTwoBid1) - (prev LegTwoAsk1)))  from tempShortLong; 
//    ShortProfit: select  from tempShortLong where Signal = -1;
//    Profit: ShortProfit, LongProfit; 
//    select  Date,Profit1,SumsProfit from update SumsProfit:sums Profit1 from `Date xasc Profit
//    }       





bollingerBands: {[k;n;data]      movingAvg: mavg[n;data];      md: sqrt mavg[n;data*data]-movingAvg*movingAvg;      movingAvg+/:(k*-1 0 1)*\:md};

f:{x%y*6f*2204.6226};
//update (Date - 1000000000*60*60*24) from `quote where Date.minute > 20:00:00 or Date.minute within 00:00:00 01:01:00;
quoteData:quoteData,update PairAsk:f[LegTwoAsk1;LegOneBid1],PairBid:f[LegTwoBid1;LegOneAsk1] from quote;
strategyData:-201#delete date,second from select by Date.date, 1 xbar Date.second from quoteData;
//delete from `strategyData where Date.minute within  01:00:00 09:00:05;
//delete from `strategyData where Date.minute within  10:15:00 10:30:05;
//delete from `strategyData where Date.minute within  11:30:00 13:30:05;
//delete from `strategyData where Date.minute within  15:00:00 21:00:05;
//update HigherBand2:bollingerBands[0.5;200;PairAsk][2],LowerBand2:bollingerBands[0.5;200;PairBid][0]  from `strategyData;
update HigherBand2:bollingerBands[evparam;200;PairAsk][2],LowerBand2:bollingerBands[evparam;200;PairBid][0]  from `strategyData;
delete from `strategyData where HigherBand2<1f or LowerBand2<1f;
Signal: strategyData;
//update Signal:`Long from `Signal where PairAsk < LowerBand2;
//update Signal:`Short from `Signal where PairBid > HigherBand2; 
update Signal:1i from `Signal where PairAsk < LowerBand2;
update Signal:-1i from `Signal where PairBid > HigherBand2; 
Signal2:select from Signal where Date = last Date;
//Signal2:select from Signal2 where ((Signal = `Long) or  (Signal = `Short));
Signal2:select from Signal2 where ((Signal = 1) or  (Signal = -1));
FinalSignal2:FinalSignal2,Signal2;
//delete from `FinalSignal2 where Date.minute within 09:00:00 09:00:05;
//delete from `FinalSignal2 where Date.minute within 10:30:00 10:30:05;
//delete from `FinalSignal2 where Date.minute within 13:30:00 13:30:05;
//delete from `FinalSignal2 where Date.minute within 21:00:00 21:00:05;
ShortLong2:select from FinalSignal2  where (Signal<>(prev Signal));
res:([]len:enlist count ShortLong2; b:-1#ShortLong2`LegTwoBid1; a:-1#ShortLong2`LegTwoAsk1;d:-1#ShortLong2`Signal);
FinalSignal2:update LowerBand2:1.0, HigherBand2:1.0 from ShortLong2;



cal:{ 
    tempShortLong: ShortLong2;
    tempShortLong:update Profit1: (((prev LegTwoBid1) - (LegTwoAsk1))) from tempShortLong; 
    LongProfit: select  from tempShortLong where Signal = 1;
    tempShortLong:update Profit1: (((LegTwoBid1) - (prev LegTwoAsk1)))  from tempShortLong; 
    ShortProfit: select  from tempShortLong where Signal = -1;
    Profit: ShortProfit, LongProfit; 
    select  Date,Profit1,SumsProfit from update SumsProfit:sums Profit1 from `Date xasc Profit
    }       

