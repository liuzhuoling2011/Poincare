/*
* tools.cpp
* Copyright(C) by MY Capital Inc. 2007-2999
*/

#include "tools.h"
#include "utils.h"

#define ROUND_UP(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

//---------------------MyArray implementation-----------------------

MyArray::MyArray(size_t size, int count)
{
	m_elem_size = size;
	m_total_count = count;
	m_data = malloc(count * size);
	if (m_data == NULL) {
		printf("MyArray malloc memory fail.\n");
	}
	m_use_count = 0;
	m_curr_pos = (char *)m_data;
	memset(m_data, 0, m_elem_size * m_total_count);
}

MyArray::~MyArray()
{
	if (m_data != NULL){
		free(m_data);
		m_data = NULL;
	}
}

MyArray::MyArray(const MyArray *other)
{
	if (other == NULL) return;
	m_elem_size = other->m_elem_size;
	m_use_count = m_total_count = other->m_use_count;
	size_t data_len = m_use_count * m_elem_size;
	m_data = malloc(data_len);
	m_curr_pos = (char *)m_data + data_len;
	memcpy(m_data, other->m_data, data_len);
}

void MyArray::clear()
{
	memset(m_data, 0, m_elem_size * m_use_count);
	m_use_count = 0;
	m_curr_pos = (char *)m_data;
}

void  MyArray::reset()
{
	m_use_count = 0;
	m_curr_pos = (char *)m_data;
}

bool 
MyArray::expand(unsigned int new_count)
{
	if (m_total_count < new_count) {
		size_t new_size = new_count * m_elem_size;
		void* new_data = realloc(m_data, new_size);
		if (new_data == NULL){
			printf("MyArray realloc fail. new size=%u\n", new_size);
			return false;
		}

		if (new_data != m_data){
			m_curr_pos = (char*)new_data + m_use_count * m_elem_size;
			m_data = new_data;
		}
		m_total_count = new_count;
	}
	return true;
}

bool 
MyArray::push(void * item)
{
	if (m_use_count >= m_total_count){
		if (!expand(m_total_count * 2))
			return false;
	}

	memcpy(m_curr_pos, item, m_elem_size);
	m_curr_pos += m_elem_size;
	m_use_count++;
	return true;
}

bool
MyArray::push(void * item, int num)
{
	size_t add_size = num * m_elem_size;
	if (m_use_count + num >= m_total_count){
		unsigned int need = m_total_count * 2;
		while (need < m_use_count + num){
			need *= 2;
		}
		if (!expand(need)) 
			return false;
	}

	memcpy(m_curr_pos, item, add_size);
	m_curr_pos += add_size;
	m_use_count += num;
	return true;
}

void * 
MyArray::next()
{
	if (m_use_count >= m_total_count){
		return NULL;
	}

	void* curr_pos = m_curr_pos;
	m_curr_pos += m_elem_size;
	m_use_count++;
	return curr_pos;
}

void 
MyArray::rollback()
{
	m_curr_pos -= m_elem_size;
	m_use_count--;
}


//---------------------HashTable implementation-----------------------
HashTable::HashTable(size_t size, int count)
{
	m_use_count = 0;
	m_elem_size = size;
	m_total_count = ROUND_UP(count);
	m_table_size = m_total_count * 2;
	m_bucket = (hash_bucket *)calloc(m_table_size, sizeof(hash_bucket));
	m_data = new MyArray(size, m_total_count);
	clear();
}

HashTable::~HashTable()
{
	free(m_bucket);
	delete m_data;
}

void HashTable::clear()
{
	for (int i = 0; i < m_table_size; i++) {
		INIT_LIST_HEAD(&m_bucket[i].head);
	}
	for (int i = 0; i < m_use_count; i++) {
		hash_base *base = (hash_base *)m_data->at(i);
		memset(base, 0, m_elem_size);
		INIT_LIST_HEAD(&base->hs_link);
	}
	m_use_count = 0;
}

void* HashTable::create()
{
	m_use_count++;
	return m_data->next();
}

void HashTable::insert(void *item, bool cover)
{
	hash_base *base = (hash_base *)item;
	if (m_use_count >= m_total_count) return;
	if (base->str_key[0] == '\0') return;

	void *have = find(base->str_key);
	if (have == NULL) {
		uint64_t index = hash_value(base->str_key);
		list_add_after(&base->hs_link, &m_bucket[index].head);
	}else{
		if (cover){
			int shift = sizeof(hash_base);
			memcpy((char *)have + shift, (char *)base + shift, m_elem_size - shift);
		}
		m_use_count--;
		m_data->rollback();
	}
}

void* HashTable::find(int num)
{
	if (num < m_total_count){
		return m_data->at(num);
	}else{
		return NULL;
	}
}

void* HashTable::find(const char* str_key)
{
	hash_base *node = NULL;
	list_head *pos, *next;
	uint64_t index = hash_value(str_key);
	list_for_each_safe(pos, next, &m_bucket[index].head) {
		node = list_entry(pos, hash_base, hs_link);
		if (my_strcmp(node->str_key, str_key) == 0) {
			return (void *)node;
		}
	}

	return NULL;
}

uint64_t HashTable::hash_value(const char *str_key)
{
	uint64_t hash = my_hash_value(str_key);
	return hash & (m_table_size - 1);
}


//---------------------RingQueue implementation-----------------------

RingQueue::RingQueue(int size, int count)
{
	m_elem_size = size;
	m_max_count = count;
	m_queue_data = malloc(count * size);
	if (m_queue_data == NULL) {
		printf("RingQuoue malloc memory fail.\n");
	}
	clear();
}

RingQueue::~RingQueue()
{
	if (m_queue_data != NULL){
		free(m_queue_data);
		m_queue_data = NULL;
	}
}

void  
RingQueue::clear()
{
	m_use_num = 0;
	m_real_count = 0;
	m_front_pos = m_rear_pos = (char *)m_queue_data;
	m_end_data = (char *)m_queue_data + m_max_count * m_elem_size;
	memset(m_queue_data, 0, m_max_count * m_elem_size);
}

void RingQueue::reset()
{
	m_use_num = 0;
	m_real_count = 0;
	m_front_pos = m_rear_pos = (char *)m_queue_data;
	for (int i = 0; i < m_max_count; i++){
		int* head_flag = (int *)((char *)m_rear_pos + i * m_elem_size);
		*head_flag = 0;
	}
}

void* 
RingQueue::push(void *node)
{
	void* curr_pos = m_rear_pos;
	memcpy(m_rear_pos, node, m_elem_size);
	if (m_rear_pos + m_elem_size >= m_end_data){
		m_rear_pos = (char *)m_queue_data;
	}else{
		m_rear_pos += m_elem_size;
	}
	m_real_count++;
	return curr_pos;
}

void * 
RingQueue::next()
{
	void* curr_pos = m_rear_pos;
	if (m_rear_pos + m_elem_size >= m_end_data){
		m_rear_pos = (char *)m_queue_data;
	}else{
		m_rear_pos += m_elem_size;
	}
	m_real_count++;
	return curr_pos;
}

void * 
RingQueue::pop()
{
	void* curr_pos = m_front_pos;
	if (m_front_pos + m_elem_size >= m_end_data){
		m_front_pos = (char *)m_queue_data;
	}else{
		m_front_pos += m_elem_size;
	}
	m_use_num++;
	return curr_pos;
}

void * RingQueue::get_node()
{
	int count = 0;
	while (count < m_max_count){
		int *flag = (int *)next();
		if (*flag == 0){
			return (void *)flag;
		}
		count++;
	}
	return NULL;
}
