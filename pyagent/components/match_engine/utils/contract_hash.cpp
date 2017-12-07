#include  <iostream>
#include  "utils.h"
#include  "contract_hash.h"
#include  "macro_def.h"

ContractHash::ContractHash()
{
	p_use_index = NULL;
	p_bucket = NULL;
	p_cont_head = NULL;
	init_hash_table();
}

ContractHash::ContractHash(ContractHash *src)
{
	p_use_index = NULL;
	p_bucket = NULL;
	p_cont_head = NULL;
	init_hash_table();
	for (int i = 0; i < src->size(); i++){
		Contract *cont = src->find(i);
		Contract *add_item = create();
		duplicate(cont, add_item);
		insert(add_item);
	}
}

ContractHash::~ContractHash()
{
	if (p_use_index != NULL){
		free(p_use_index);
		p_use_index = NULL;
	}
	if (p_cont_head != NULL){
		free(p_cont_head);
		p_cont_head = NULL;
	}
}

void
ContractHash::init_hash_table()
{
	char *ht = NULL;
	p_use_index = (char *)malloc(MAX_ITEM_SIZE);

	int size = MAX_ITEM_SIZE * sizeof(Contract) +
		       HASH_TABLE_SIZE * sizeof(hash_bucket);

	void * all_mem = calloc(1, size);
	if (all_mem == NULL) {
		PRINT_ERROR("init_hash_table malloc memory fail.");
		return;
	}

	ht = (char *)all_mem;
	p_cont_head = (Contract *)(all_mem);
	p_bucket = (hash_bucket *)(ht + MAX_ITEM_SIZE * sizeof(Contract));
	clear();
}

void
ContractHash::init_position(Position &post)
{
	memset(&post, 0, sizeof(Position));
}

void
ContractHash::init_contract(Contract *cont)
{
	memset(cont, 0, sizeof(Contract));
	cont->exch = UNDEFINED_EXCH;
	cont->rank = UNDEFINED_RANK;
	cont->security_type = FUTURE;
	cont->tick_size = 0;
	cont->multiplier = 1;
	cont->fee_by_lot = true;
	cont->m_pending_buy_open_size = 0;
	cont->m_pending_sell_open_size = 0;
	cont->m_pending_buy_close_size = 0;
	cont->m_pending_sell_close_size = 0;
	cont->m_order_no_entrusted_num = 0;
}

void
ContractHash::clear()
{
	m_use_count = 0;
	memset(p_use_index, 0, MAX_ITEM_SIZE);

	for (int i = 0; i < HASH_TABLE_SIZE; i++){
		INIT_LIST_HEAD(&p_bucket[i].head);
	}

	for (int i = 0; i < MAX_ITEM_SIZE; i++){
		init_contract(p_cont_head+i);
		INIT_LIST_HEAD(&p_cont_head[i].link);
		INIT_LIST_HEAD(&p_cont_head[i].hs_link);
	}
}

uint64_t
ContractHash::hash_value(const char *str_key)
{
	uint64_t hash = my_hash_value(str_key);
	return hash & (HASH_TABLE_SIZE - 1);
}


Contract *
ContractHash::create()
{
	if (m_use_count < MAX_ITEM_SIZE){
		return &p_cont_head[m_use_count];
	}else{
		return NULL;
	}
}

void 
ContractHash::insert(Contract *cnt)
{
	if (m_use_count >= MAX_ITEM_SIZE) return;
	if (cnt->symbol[0] == '\0') return;

	/* check existing key value pair */
	if (find(cnt->symbol) == NULL) {
		uint64_t hv = hash_value(cnt->symbol);
		list_add_after(&cnt->hs_link, &p_bucket[hv].head);
		p_use_index[m_use_count] = 1;
		m_use_count++;
	}
}

void
ContractHash::erase(int index)
{
	Contract *cnt = &p_cont_head[index];
	list_del(&cnt->hs_link);
	p_use_index[index] = 0;

	// FIXME
	// although p_cont_head[index] is not used and p_use_index[index] is set to 0,
	// get_empty_item() still returns the tail item
}

Contract *
ContractHash::find(const char *symbol, bool is_create)
{
	Contract *cnt;
	struct list_head *pos, *n;

	uint64_t hv = hash_value(symbol);
	list_for_each_safe(pos, n, &p_bucket[hv].head){
		cnt = list_entry(pos, Contract, hs_link);
		if (my_strcmp(cnt->symbol, symbol) == 0) {
			goto end;
		}
	}
	
	cnt = is_create ? create() : NULL;
end:
	return cnt;
}

Contract * ContractHash::find_product(const char * product, bool forward)
{
	Contract *cnt;
	if (forward == true) {
		for (int i = 0; i < m_use_count; i++) {
			cnt = find(i);
			if (strcmp_case(cnt->product, product) == 0) {
				return cnt;
			}
		}
	}
	else {
		for (int i = m_use_count - 1; i >= 0; i--) {
			cnt = find(i);
			if (strcmp_case(cnt->product, product) == 0) {
				return cnt;
			}
		}
	}
	return nullptr;
}

void ContractHash::find_product(const char * product, std::vector<Contract *> &contracts)
{
	Contract *cnt;
	for (int i = m_use_count - 1; i >= 0; i--) {
		cnt = find(i);
		if (strcmp_case(cnt->product, product) == 0) {
			contracts.push_back(cnt);
		}
	}
}

void ContractHash::find_group_id(int group_id, std::vector<Contract*>& contracts)
{
	for (int i = 0; i < m_use_count; i++) {
		Contract *cnt = find(i);
		for (int k = 0; k < cnt->group_num; k++){
			if (cnt->group_ids[k] == group_id) {
				contracts.push_back(cnt);
				break;
			}
		}
	}
}

Contract *
ContractHash::find(int num)
{
	if (num < MAX_ITEM_SIZE){
		return &p_cont_head[num];
	}else{
		return NULL;
	}
}

bool
ContractHash::is_empty()
{
	for (int i = 0; i < m_use_count; i++){
		if (p_use_index[i] == 1)
			return false;
	}
	return true;
}

void add_contract_group_id(Contract *dest, int group_id)
{
	for (int i = 0; i < dest->group_num; i++){
		if (dest->group_ids[i] == group_id)
			return;
	}

	if (dest->group_num < MAX_GROUP_SIZE){
		dest->group_ids[dest->group_num] = group_id;
		dest->group_num++;
	}else{
		PRINT_ERROR("add_contract_group_id: overflow!");
	}
}

void duplicate(Contract *src, Contract *dest)
{
	dest->exch = src->exch;
	dest->rank = src->rank;
	dest->rank_type = src->rank_type;
	dest->max_pos = src->max_pos;
	dest->source = src->source;
	dest->max_accum_open_vol = src->max_accum_open_vol;
	dest->single_side_max_pos = src->single_side_max_pos;
	strlcpy(dest->product, src->product, MAX_SYMBOL_SIZE);
	strlcpy(dest->symbol, src->symbol, MAX_SYMBOL_SIZE);

	dest->type_num = src->type_num;
	for (int i = 0; i < src->type_num; i++){
		dest->feed_types[i] = src->feed_types[i];
	}

	for (int i = 0; i < src->group_num; i++){
		add_contract_group_id(dest, src->group_ids[i]);
	}
}

//----------positions related-------
double 
avg_px(Position &pos)
{
	if (pos.qty > 0)
		return pos.notional / (double)pos.qty;
	else
		return 0.0;
}

void
add_feed_type(Contract *cont, int type)
{
	if (cont->type_num >= 0 && cont->type_num < 8){
		for (int i = 0; i < cont->type_num; i++) {
			if (cont->feed_types[i] == type){
				/* check if type already added to avoid duplicates*/
				return;
			}
		}

		cont->feed_types[cont->type_num] = type;
		cont->type_num++;
	}
}

int
position(Contract *cont)
{
	return long_position(cont) - short_position(cont);
}

int
long_position(Contract *cont)
{
	return cont->positions[LONG_OPEN].qty - cont->positions[SHORT_CLOSE].qty;
}

int
short_position(Contract *cont)
{
	return cont->positions[SHORT_OPEN].qty - cont->positions[LONG_CLOSE].qty;
}

double
long_notional(Contract *cont)
{
	return cont->positions[LONG_OPEN].notional + cont->positions[LONG_CLOSE].notional;
}

double
short_notional(Contract *cont)
{
	return cont->positions[SHORT_OPEN].notional + cont->positions[SHORT_CLOSE].notional;
}

double
avg_buy_price(Contract *cont)
{
	return (cont->positions[LONG_OPEN].notional + cont->positions[LONG_CLOSE].notional) 
		/ (cont->positions[LONG_OPEN].qty + cont->positions[LONG_CLOSE].qty);
}

double
avg_sell_price(Contract *cont)
{
	return (cont->positions[SHORT_OPEN].notional + cont->positions[SHORT_CLOSE].notional) 
		/ (cont->positions[SHORT_OPEN].qty + cont->positions[SHORT_CLOSE].qty);
}

double
get_transaction_fee(Contract *cont, int size, double price, bool flag_close_yes)
{
	double l_fee = 0.0;
	double exchange_fee = 0.0;
	if (flag_close_yes){
		exchange_fee = cont->yes_exchange_fee;
	}
	else{
		exchange_fee = cont->exchange_fee;
	}
	if (cont->fee_by_lot)
		l_fee = size * (exchange_fee + cont->broker_fee); // Caution, for futures right now, broker fee is 0.0
	else
		l_fee = size * price * (exchange_fee + cont->broker_fee);
	if (cont->exch == SSE)
		l_fee += size * price * cont->acc_transfer_fee;

	return l_fee;
}

double
get_realized_PNL(Contract *cont)
{
	double long_side_PNL = 0.0;
	double short_side_PNL = 0.0;

	int long_pos = MIN(cont->positions[LONG_OPEN].qty, cont->positions[SHORT_CLOSE].qty);
	int short_pos = MIN(cont->positions[SHORT_OPEN].qty, cont->positions[LONG_CLOSE].qty);

	if (long_pos > 0)
		long_side_PNL = long_pos * (avg_px(cont->positions[SHORT_CLOSE]) - avg_px(cont->positions[LONG_OPEN]));

	if (short_pos > 0)
		short_side_PNL = short_pos * (avg_px(cont->positions[SHORT_OPEN]) - avg_px(cont->positions[LONG_CLOSE]));

	return long_side_PNL + short_side_PNL;
}

double
get_unrealized_PNL(Contract *cont)
{
	double long_side_unrealized_PNL = 0.0;
	double short_side_unrealized_PNL = 0.0;
	int long_pos = long_position(cont);
	int short_pos = short_position(cont);

	if (compare(cont->last_px, 0) == 0) {
		PRINT_WARN("%s Contract last_price is 0", cont->symbol);
		return 0.0;
	}

	if (long_pos >= 0)
		long_side_unrealized_PNL = long_pos * (cont->last_px - avg_px(cont->positions[LONG_OPEN]));
	else
		PRINT_ERROR("Sell Close Qty is higher than Buy Open, something is wrong");

	if (short_pos >= 0)
		short_side_unrealized_PNL = short_pos * (avg_px(cont->positions[SHORT_OPEN]) - cont->last_px);
	else
		PRINT_ERROR("Long Close Qty is higher than Sell Open, something is wrong");

	return long_side_unrealized_PNL + short_side_unrealized_PNL;
}

double
get_contract_PNL(Contract *cont)
{
	return get_realized_PNL(cont) + get_unrealized_PNL(cont);
}

/* Suppose we start from 0, Cash is the PNL + current position cost */
// Only used in H5H78 and other strategy, CAUTION!!! It's not cash, it's in points.
double
get_contract_Cash(Contract *cont)
{
	double long_side_cash = 0.0;
	double short_side_cash = 0.0;

	long_side_cash = -1 * (cont->positions[LONG_OPEN].notional + cont->positions[LONG_CLOSE].notional);
	short_side_cash = cont->positions[SHORT_OPEN].notional + cont->positions[SHORT_CLOSE].notional;

	return long_side_cash + short_side_cash;
}