#include  <assert.h>
#include  <fstream>
#include  "match_engine.h"
#include  "extend_match.h"
#include  "my_trade.h"
#include  "list.h"
#include  "utils.h"
#include  "json11.hpp"
using namespace json11;

#define  HASH_TABLE_SIZE      (8192)
#define  MAX_ITEM_SIZE        (4096)
#define  THRESHOLD_SIZE       (2)
#define  MAX_RESP_SIZE        (4*1024*1024)

struct match_node{
	char           symbol[MAX_NAME_SIZE];
	bool           is_clear;
	MatchEngine   *match;
	list_head      hs_link;
};

//---------------------EngineHash implementation-----------------------
EngineHash::EngineHash()
{
	m_use_count = 0;
	m_max_count = 0;
	int size = MAX_ITEM_SIZE * sizeof(match_node) +
		       HASH_TABLE_SIZE * sizeof(hash_bucket);
	void * all_mem = calloc(1, size);
	assert(all_mem);

	char *table = (char *)all_mem;
	m_match_head = (match_node *)(all_mem);
	m_bucket = (hash_bucket *)(table + MAX_ITEM_SIZE * sizeof(match_node));
	for (int i = 0; i < HASH_TABLE_SIZE; i++){
		INIT_LIST_HEAD(&m_bucket[i].head);
	}
	for (int i = 0; i < m_use_count; i++){
		match_node *node = m_match_head + i;
		INIT_LIST_HEAD(&node->hs_link);
	}
}

EngineHash::~EngineHash()
{
	for (int i = 0; i < MAX_ITEM_SIZE; i++){
		match_node *node = m_match_head + i;
		if (node->match != NULL){
			delete node->match;
			node->match = NULL;
		}
	}
	if (m_match_head != NULL){
		free(m_match_head);
		m_match_head = NULL;
	}
}

void
EngineHash::clear()
{
	for (int i = 0; i < m_use_count; i++){
		match_node *node = m_match_head + i;
		node->is_clear = true;
		node->match->clear();
	}
}

match_node *
EngineHash::add(char* symbol, product_info *product, mode_config *mode, bool is_bar)
{
	if (g_unlikely(m_use_count >= MAX_ITEM_SIZE)) 
		return NULL;

	match_engine_config me_cfg = { 0 };
	strlcpy(me_cfg.contract, symbol, MAX_NAME_SIZE);
	memcpy(&me_cfg.pi, product, sizeof(product_info));
	memcpy(&me_cfg.mode_cfg, mode, sizeof(mode_config));

	match_node *node = find(symbol);
	if (node == NULL){
		node = &m_match_head[m_use_count];
		node->is_clear = true;
		strlcpy(node->symbol, symbol, MAX_NAME_SIZE);
		uint64_t index = hash_value(symbol);
		list_add_after(&node->hs_link, &m_bucket[index].head);
		m_use_count++;
		m_max_count = m_use_count;
	}

	if (node->match == NULL){
		if (is_bar || mode->mode == BAR_CLOSE_MATCH){
			node->match = new BarCloseMatch(symbol);
		}else{
			if (mode->mode == NORMAL_MATCH){
				node->match = new MatchEngine(symbol);
			}else if (mode->mode == IDEAL_MATCH){
				node->match = new IdealMatch(symbol);
			}else if (mode->mode == LINEAR_IMPACT_MATCH){
				node->match = new LinearMatch(symbol);
			}else if (mode->mode == DOUBLE_GAUSSIAN_MATCH){
				node->match = new GaussianMatch(symbol);
			}else if (mode->mode == SPLIT_TWO_PRICE_MATCH){
				node->match = new OriginMatch(symbol);
			}else{
				printf("Match engine mode error!");
				assert(node->match);
			}
		}
	}

	node->is_clear = false;
	node->match->set_config(&me_cfg);
	
	return node;
}

match_node *
EngineHash::find(char* symbol)
{
	if (m_use_count > THRESHOLD_SIZE){
		return hash_find(symbol);
	}else{
		return traverse_find(symbol);
	}
}

match_node *
EngineHash::find(int index)
{
	return m_match_head + index;
}

match_node *
EngineHash::hash_find(char* symbol)
{
	list_head *pos, *next;
	uint64_t index = hash_value(symbol);
	list_for_each_safe(pos, next, &m_bucket[index].head){
		match_node *node = list_entry(pos, match_node, hs_link);
		if (my_strcmp(node->symbol, symbol) == 0) {
			return node;
		}
	}
	return NULL;
}

match_node *
EngineHash::traverse_find(char* symbol)
{
	for (int i = 0; i < m_use_count; i++){
		match_node *node = m_match_head + i;
		if (my_strcmp(node->symbol, symbol) == 0)
			return node;
	}
	return NULL;
}

uint64_t
EngineHash::hash_value(char *symbol)
{
	uint64_t hash = my_hash_value(symbol);
	return hash & (HASH_TABLE_SIZE - 1);
}


//---------------------TradeHandler implementation-----------------------

TradeHandler::TradeHandler(int match_type)
{
	m_pi_cnt = 0;
	m_match_type = match_type;
	m_pi_data = (product_info *)malloc(MAX_ITEM_SIZE * sizeof(product_info));
	m_engine = new EngineHash();
	m_mode_cfg = new MyArray(sizeof(mode_config), MAX_NAME_SIZE);
}

TradeHandler::~TradeHandler()
{
	free(m_pi_data);
	delete m_engine;
	delete m_mode_cfg;
}

void TradeHandler::clear()
{
	if (m_engine->size() > 0){
		m_engine->clear();
	}
}

int
TradeHandler::load_product(product_info* pi, int count)
{
	if (g_unlikely(NULL == pi || count <= 0)) {
		return -1;
	}
	int max_len = count < MAX_ITEM_SIZE ? count : MAX_ITEM_SIZE;
	memcpy(m_pi_data, pi, max_len * sizeof(product_info));
	m_pi_cnt = max_len;
	return 0;
}

int  
TradeHandler::load_config(mode_config * m_cfg, int count)
{
	if (m_cfg != NULL && count > 0){
		m_mode_cfg->push((void *)m_cfg, count);
	}
	return 0;
}

product_info* 
TradeHandler::default_product(char* symbol, char *prd, Exchanges exch)
{
	bool finded = false;
	strlcpy(m_default_pi.name, symbol, MAX_NAME_SIZE);
	strlcpy(m_default_pi.prefix, symbol, MAX_NAME_SIZE);
	m_default_pi.xchg_code = exch;

	if (exch == SSE){
		m_default_pi.acc_transfer_fee = ACC_TRANSFER_FEE; //�����ѣ����Ͻ�����ȡ
		m_default_pi.rate = BROKER_FEE;     //Ӷ��
		m_default_pi.stamp_tax = STAMP_TAX; //�����ӡ��˰
		m_default_pi.unit = 0.01;       //��С�䶯��λ
		m_default_pi.multiple = 1;    //���׵Ļ�����
		m_default_pi.fee_fixed = 1;     //�Ƿ�̶�����
		finded = true;
	}else if (exch == SZSE){
		m_default_pi.rate = BROKER_FEE;
		m_default_pi.stamp_tax = STAMP_TAX;
		m_default_pi.unit = 0.01;      
		m_default_pi.multiple = 1;    
		m_default_pi.fee_fixed = 1;
		finded = true;
	} else {
		PRINT_WARN("Warning: exch %d, Can't find [%s] fee info!\n", exch, symbol);
	}
	return  &m_default_pi;
}

product_info*
TradeHandler::find_product(char* symbol, Exchanges exch)
{
 	char product[MAX_SYMBOL_SIZE] = { 0 };
	// Find product from Symbol, then compare with fee field (prefix)
	get_product_by_symbol(symbol, product);
	
	for (int idx = 0; idx < m_pi_cnt; idx++){
		product_info* pi = m_pi_data + idx;
        // TODO: add exchange, but is unnecessary now
		if (strcmp_case(product, pi->prefix) == 0) {
		 //&& pi->xchg_code == exch){
			return pi;
		}
	}

	return default_product(symbol, product, exch);
}

match_node *
TradeHandler::find_engine(char* symbol, Exchanges exch, bool is_bar)
{
	match_node *node = m_engine->find(symbol);
	if (g_unlikely(node == NULL || node->is_clear)){
		product_info *product = find_product(symbol, exch);
		//assert(product);
		mode_config *mode = find_mode_config(product);
		node = m_engine->add(symbol, product, mode, is_bar);
		//assert(node);
	}
	return node;
}

mode_config * 
TradeHandler::find_mode_config(product_info* prd)
{
	mode_config *node = NULL, *any_node = NULL;
	for (unsigned int i = 0; i < m_mode_cfg->size(); i++){
		node = (mode_config *)m_mode_cfg->at(i);
		if (node->mode == m_match_type && node->exch == prd->xchg_code){
			if (strcmp_case(node->product, prd->prefix) == 0){
				return node;
			}
			else if (strcmp_case(node->product, "any") == 0){
				any_node = node;
			}
		}
	}
	if (any_node != NULL){
		return any_node;
	}else{
		//PRINT_WARN("Can't find [mode=%d product=%s] mode config!", m_match_type, prd->prefix);
		m_default_cfg.mode = m_match_type;
		m_default_cfg.trade_ratio = 0.2;
		// Only used for Linear Impact Model
		m_default_cfg.param_num = 3;
		m_default_cfg.params[0] = 1000;
		m_default_cfg.params[1] = 1;
		m_default_cfg.params[2] = 80;

		return &m_default_cfg;
	}
}

bool TradeHandler::is_empty_order()
{
	bool is_empty = true;
	match_node *node = NULL;
	for (int i = 0; i < m_engine->size(); i++){
		node = m_engine->find(i);
		is_empty = node->match->is_empty_order();
		if (!is_empty)  return false;
	}
	return true;
}

void 
TradeHandler::place_order(order_request* pl_ord, signal_response *ord_rtn)
{
	match_node *node = find_engine(pl_ord->symbol, pl_ord->exchange);
	node->match->place_order(pl_ord, ord_rtn);
	mat_quote *curr_qut = node->match->get_current_quote(true);
}

void 
TradeHandler::cancel_order(order_request* cl_ord, signal_response *ord_rtn)
{
	match_node *node = find_engine(cl_ord->symbol, cl_ord->exchange);
	node->match->cancel_order(cl_ord, ord_rtn);
	mat_quote *curr_qut = node->match->get_current_quote(true);
}

void 
TradeHandler::feed_quote(void* quote, int mi_type, signal_response** trd_rtn_ar, 
              int* rtn_cnt, int market, bool is_bar)
{
	Exchanges exch_code;
	char* symbol = NULL;
	bool is_stock = false;
	if(mi_type == MI_MY_STOCK_LV2 || mi_type == MI_MY_STOCK_INDEX || mi_type == MI_SGD_OPTION) {
		is_stock = true;
	}

	if(is_bar) {
		Internal_Bar *bar = (Internal_Bar*)quote;
		symbol = bar->symbol;

		//Contract * l_contract = m_contracts->find(symbol);
		//if (l_contract != NULL) {
		//	exch_code = l_contract->exch;
		//} else {
		//	exch_code = UNDEFINED_EXCH;
		//}
		exch_code = UNDEFINED_EXCH;
	} else {
		if(is_stock == true) {
			Stock_Internal_Book *book = (Stock_Internal_Book *)quote;
			symbol = book->ticker;
            if (book->wind_code[8] == 'H') {
                exch_code = SSE;
            }
            else if(book->wind_code[8] == 'Z') {
                exch_code = SZSE;
            } else {
			    exch_code = UNDEFINED_EXCH;
            }
		} else {
			Futures_Internal_Book *book = (Futures_Internal_Book *)quote;
			symbol = book->symbol;
			exch_code = (Exchanges)book->exchange;
		}
	}

	
	match_node *node = find_engine(symbol, exch_code, is_bar);
	if(is_stock) {
		node->match->feed_stock_quote(mi_type, quote, market, is_bar);
	} else {
		node->match->feed_future_quote(mi_type, quote, market, is_bar);
	}
	node->match->execute_match((void **)trd_rtn_ar, rtn_cnt);
}
