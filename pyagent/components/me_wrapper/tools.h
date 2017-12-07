/*
* tools.h
* Implementation of the general data structure
* Copyright(C) by MY Capital Inc. 2007-2999
*/

#pragma once
#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <assert.h>
#include "list.h"
#include "utils.h"

#define  TIME_OUT   500

/**
* @class   SdpArray
*
* @brief   High performance dynamic array to store POD structs.
*          We believe MyArray provides an unparalleled combination of performance and memory usage.
*
* @notice  1. Tell SdpArray max size you need in constructor to avoid memory reallocated
*          2. Recommand to use next() to get full performace
*          3. We should use insert && erase as little as possible, it will spent much time to move memory.
*          4. Don't point to item in SdpArray, every reallocate may make pointers invalid.
*
* @author  savion
* @since   20160829
*
* @reviser liuzhuoling
* @date    20170912 update vector interface
* @date    20170914 template refactor
*
*/


template<class T>

class SdpArray {

public:
	/**
	* Create an array to contain items. Preallocate space enough for `count` items.
	*
	* @param[in]  count  initial capacity
	*/
	SdpArray(size_t count = 4) {
		m_elem_size = sizeof(T);
		m_data = (T*)malloc(count * m_elem_size);
		m_use_count = 0;
		m_total_count = count;
		m_last_pos = m_data;
	}

	void operator= (const SdpArray<T> &other) {
		m_elem_size = other.m_elem_size;
		m_use_count = other.m_use_count;
		m_total_count = other.m_total_count;
		size_t data_len = m_use_count * m_elem_size;

		if (m_data != NULL) {
			T* new_data = (T*)realloc(m_data, data_len);
			if (new_data != m_data) {
				m_data = new_data;
			}
		}

		m_last_pos = m_data + m_use_count;
		memcpy(m_data, other.m_data, data_len);
	}

	T& operator[] (size_t pos) {
		return *at(pos);
	}

	~SdpArray() {
		if (m_data != NULL) {
			free(m_data);
			m_data = NULL;
		}
	}

	/*------------ Capacity Related ------------*/
	/**
	* Resize the array to a new capacity.
	*
	* @param[in]  count   new capacity to expan
	*/
	void resize(size_t count) {
		T* new_data = (T*)realloc(m_data, count * m_elem_size);

		if (new_data != m_data) {
			m_data = new_data;
		}
		if (m_use_count > count) {
			m_use_count = count;
		}
		m_total_count = count;
		m_last_pos = m_data + m_use_count;
	}

	/**
	* Expand the array to a new capacity.
	* If the current capacity is greater than the new capacity, nothing is done.
	*
	* @param[in]  count   new capacity to expan
	*/
	void reserve(size_t count) {
		if (m_total_count < count) {
			resize(count);
		}
	}

	/**
	* @return  size of elements in array
	*/
	size_t size() { return m_use_count; }

	/**
	* @return  max size of the array
	*/
	size_t capacity() { return m_total_count; }

	/**
	* @return  return true when array is empty
	*/
	bool empty() { return m_use_count == 0; }

	/*------------ Element Access Related ------------*/
	/**
	* @return  index position item address of the array
	*/
	T* at(size_t index) { return m_data + index; }

	/**
	* @return  first item address of the array
	*/
	T& front() { return *m_data; }

	/**
	* @return  last item address of the array
	*/
	T& back() { return *(m_last_pos - 1); }

	/**
	* @return  point to the next free item, recommand to use!
	*/
	T& next() {
		if (m_use_count >= m_total_count) {
			resize(m_total_count * 2);
		}

		m_use_count++;
		m_last_pos++;
		return *(m_last_pos - 1);
	}

	/*------------ Modifiers Related ------------*/
	/**
	* Copy an item to the end of this array.
	* The item should have the same data structure as other items in this array.
	*
	* @param[in]  item   item to be inserted to the end of this array
	*/
	void push_back(T& item) {
		if (m_use_count == m_total_count) {
			resize(m_total_count * 2);
		}
		*m_last_pos = item;
		m_last_pos++;
		m_use_count++;
	}

	/**
	* Copy an array of items to the end of this array.
	* Items in the source array and this array should have the same data structure.
	*
	* @param[in]  item   starting pointer of another array
	* @param[in]  num    number of items to copy
	*/
	void push_back(T* item, size_t count = 1) {
		if (m_use_count + count >= m_total_count) {
			size_t need = m_total_count * 2;
			while (need < m_use_count + count) {
				need *= 2;
			}
			resize(need);
		}
		memcpy(m_last_pos, item, count * m_elem_size);
		m_last_pos += count;
		m_use_count += count;
	}

	/**
	* Pop items from the end of this array.
	*
	* @param[in]  count  numble of item to be poped
	*/
	void pop_back(size_t num = 1) {
		m_use_count -= num;
		m_last_pos -= num;
	}

	/**
	* Insert an item in specific position of the array.
	*
	* @param[in]  index  insert position
	* @param[in]  item   item to be inserted in index position
	*/
	void insert(size_t index, T& item) {
		if (index > m_total_count) return;

		if (m_total_count == m_use_count) {
			resize(m_total_count * 2);
		}

		for (size_t i = m_use_count - 1; i >= index; i--) {
			*at(i + 1) = *at(i);
		}

		*at(index) = item;

		m_use_count++;
		m_last_pos++;
	}

	/**
	* Insert items in specific position of the array.
	*
	* @param[in]  index  insert position
	* @param[in]  item   starting pointer of another array
	* @param[in]  count  number of items to copy
	*/
	void insert(size_t index, T* item, size_t count = 1) {
		if (index > m_total_count) return;

		if (m_use_count + count >= m_total_count) {
			size_t need = m_total_count * 2;
			while (need < m_use_count + count) {
				need *= 2;
			}
			resize(need);
		}

		for (size_t i = m_use_count - 1; i >= index; i--) {
			*at(i + count) = *at(i);
		}

		size_t add_size = count * m_elem_size;
		memcpy(at(index), item, add_size);

		m_use_count += count;
		m_last_pos += count;
	}

	/**
	* Erase items in specific position of the array.
	*
	* @param[in]  index  erase position
	*/
	void erase(size_t index) {
		if (index > m_use_count) return;

		for (size_t i = index; i < m_use_count; i++) {
			*at(i) = *at(i + 1);
		}
		m_use_count--;
		m_last_pos--;
	}

	/**
	* Erase items in specific position of the array.
	*
	* @param[in]  start  start position
	* @param[in]  end    end position
	*/
	void erase(size_t start, size_t end) {
		if (start > end || start < 0 || end > m_use_count) return;

		int size = end - start + 1;
		if (start == end) {
			erase(start);
		}
		else {
			for (size_t i = start; i < m_use_count - size; i++) {
				*at(i) = *at(i + size);
			}
			m_use_count -= size;
			m_last_pos -= size;
		}
	}

	/**
	* Clear all data
	*/
	void clear() {
		memset(m_data, 0, m_elem_size * m_use_count);
		m_use_count = 0;
		m_last_pos = m_data;
	}

	/**
	* Reset counter, not clear data
	*/
	void reset() {
		m_use_count = 0;
		m_last_pos = m_data;
	}

private:
	T*     m_data;
	T*     m_last_pos = NULL;
	size_t m_elem_size = 0;
	size_t m_use_count = 0;
	size_t m_total_count = 0;
};

/**
* @class   SdpHash
*
* @brief   Provide a general hash table.
*          Support the (string->value) mapping relationship
*          We believe SdpHash provides an unparalleled combination of performance and memory usage.
*
* @notice  1. Tell SdpHash the max size you need in constructor to avoid memory reallocated
*          2. Recommand to use next_free_node() to avoid copy object twice
*          4. Don't point to item in SdpHash, every reallocate may make pointers invalid.
*
* @author  liuzhuoling
* @date    20170918
*
*/

#define DEFAULT_HASH_SIZE 256
#define HASH_KEY_SIZE     256
#define ROUND_UP(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

template <class V>
class SdpHash {
public:
	struct HashNode {
		char key[HASH_KEY_SIZE];
		V    value;
		list_t hash_link;
		list_t free_link;
		list_t used_link;
	};

public:
	SdpHash(size_t size = DEFAULT_HASH_SIZE) {
		m_hash_size = ROUND_UP(size);
		m_data = (HashNode*)malloc(m_hash_size * sizeof(HashNode));
		m_hash_head = (list_t*)malloc(m_hash_size * sizeof(list_t));

		clear();
	}

	~SdpHash() {
		if (m_data != NULL) {
			free(m_data); m_data = NULL;
		}
		if (m_hash_head != NULL) {
			free(m_hash_head); m_hash_head = NULL;
		}
	}

	V& operator[] (const char* key) {
		return at(key);
	}

	/*------------ Capacity Related ------------*/
	/**
	* @return  size of elements in array
	*/
	size_t size() { return m_use_count; }

	/**
	* @return  return true when array is empty
	*/
	bool empty() { return m_use_count == 0; }

	/*------------ Element Access Related ------------*/
	V& at(const char* key) {
		HashNode *l_node = query(key);
		if (l_node == NULL) {
			assert(false);
			return *((V*)NULL);
		}
		else
			return l_node->value;
	}

	bool exist(const char* key) {
		HashNode *l_node = query(key);
		if (l_node == NULL)
			return false;
		else
			return true;
	}

	/*------------ Modifiers Related ------------*/
	void insert(const char* key, V& value) {
		HashNode *l_node = query(key);
		if (l_node == NULL) {
			HashNode *l_node = get_free_node();

			size_t hash_value = get_hash_value(key);
			list_add_after(&l_node->hash_link, &m_hash_head[hash_value]);
			list_add_after(&l_node->used_link, &m_used_head);

			strlcpy(l_node->key, key, HASH_KEY_SIZE);
			l_node->value = value;

			m_use_count++;
		}
		else {
			l_node->value = value;
		}
	}

	void erase(const char* key) {
		HashNode *l_node = query(key);
		if (l_node == NULL) return;

		list_del(&l_node->hash_link);
		list_del(&l_node->used_link);
		list_add_after(&l_node->free_link, &m_free_head);

		m_use_count--;
	}

	void clear() {
		m_use_count = 0;
		INIT_LIST_HEAD(&m_free_head);
		INIT_LIST_HEAD(&m_used_head);
		for (size_t i = 0; i < m_hash_size; i++) {
			INIT_LIST_HEAD(&m_hash_head[i]);
		}
		for (size_t i = 0; i < m_hash_size; i++) {
			INIT_LIST_HEAD(&m_data[i].hash_link);
			INIT_LIST_HEAD(&m_data[i].used_link);
			list_add_after(&m_data[i].free_link, &m_free_head);
		}
	}

	void init_iterator() {
		count_ = 0;
		m_cur_pos = m_used_head.prev;
	}

	bool next_used_node(char* key, V* value) {
		if (m_cur_pos != &m_used_head) {
			//printf("m_cur_pos: %p  &m_used_head: %p  m_cur_pos->pre: %p  m_cur_pos->next: %p  ", m_cur_pos, &m_used_head, m_cur_pos->prev, m_cur_pos->next);
			HashNode *l_node = list_entry(m_cur_pos, HashNode, used_link);
			m_cur_pos = m_cur_pos->prev;
			strlcpy(key, l_node->key, HASH_KEY_SIZE);
			*value = l_node->value;
			return true;
		}
		return false;
	}

	bool next_used_node(V* value) {
		if (m_cur_pos != &m_used_head) {
			HashNode *l_node = list_entry(m_cur_pos, HashNode, used_link);
			m_cur_pos = m_cur_pos->prev;
			*value = l_node->value;
			return true;
		}
		return false;
	}

	V& next_free_node() {
		HashNode *l_node = get_free_node();
		m_cur_free_node = l_node;
		return l_node->value;
	}

	void insert_exist_node(const char* key) {
		HashNode *l_node = query(key);
		if (l_node == NULL) {
			size_t hash_value = get_hash_value(key);
			list_add_after(&m_cur_free_node->hash_link, &m_hash_head[hash_value]);
			list_add_after(&m_cur_free_node->used_link, &m_used_head);

			strlcpy(m_cur_free_node->key, key, HASH_KEY_SIZE);
			m_use_count++;
		}
	}

private:
	size_t get_hash_value(const char* key) {
		return my_hash_value(key) & (m_hash_size - 1);
	}

	void expand_size() {
		// Init hash, used, free list, and then insert previous node
		INIT_LIST_HEAD(&m_used_head);
		INIT_LIST_HEAD(&m_free_head);

		list_t* new_hash_head = (list_t*)malloc(2 * m_hash_size * sizeof(list_t));
		free(m_hash_head);
		m_hash_head = new_hash_head;
		for (size_t i = 0; i < 2 * m_hash_size; i++) {
			INIT_LIST_HEAD(&m_hash_head[i]);
		}

		HashNode* new_data = (HashNode*)malloc(2 * m_hash_size * sizeof(HashNode));
		memcpy(new_data, m_data, m_hash_size * sizeof(HashNode));
		free(m_data);
		m_data = new_data;

		for (size_t i = 0; i < m_hash_size; i++) {
			m_cur_free_node = &m_data[i];
			insert_exist_node(m_cur_free_node->key);
		}

		for (size_t i = m_hash_size; i < 2 * m_hash_size; i++) {
			INIT_LIST_HEAD(&m_data[i].hash_link);
			INIT_LIST_HEAD(&m_data[i].used_link);
			list_add_after(&m_data[i].free_link, &m_free_head);
		}

		m_hash_size *= 2;
	}

	HashNode* query(const char* key) {
		struct list_head *pos, *n;
		HashNode *l_node;

		size_t hash_value = get_hash_value(key);

		list_for_each_safe(pos, n, &m_hash_head[hash_value]) {
			l_node = list_entry(pos, HashNode, hash_link);
			if (my_strcmp(l_node->key, key) == 0) {
				return l_node;
			}
		}

		return NULL;
	}

	HashNode* get_free_node() {
		if (m_free_head.next == &m_free_head) {
			expand_size();
		}
		HashNode* l_node = list_entry(m_free_head.next, HashNode, free_link);
		list_del(&l_node->free_link);
		INIT_LIST_HEAD(&l_node->free_link);
		return l_node;
	}

private:
	int count_ = 0;
	size_t  m_use_count = 0;
	size_t  m_hash_size = 0;
	list_t *m_cur_pos = NULL;
	list_t  m_free_head;
	list_t  m_used_head;
	list_t *m_hash_head = NULL;
	HashNode *m_cur_free_node = NULL; // uesd for high performance
	HashNode *m_data = NULL;
};

/**
 * @class  MyArray
 *
 * @brief  High performance dynamic array to store POD structs.
 *
 * @author savion
 * @since  29 August 2016
 */
class MyArray{

public:
	/**
	 * Create an array to contain items of size `size`. Preallocate space enough for `count` items.
	 * 
	 * @param[in]  size   size of a single item
	 * @param[in]  count  initial capacity
	 */
	MyArray(size_t size, int count);

	/* */
	~MyArray();

	/**
	 * Copy constructor.
	 */
	MyArray(const MyArray *other);

	/**
	 * Copy an item to the end of this array.
	 * The item should have the same data structure as other items in this array.
	 * 
	 * @param[in]  item   item to be copied to the end of this array
	 * @return            true - success; false - fail
	 */
	bool   push(void * item);

	/**
	 * Copy an array of items to the end of this array.
	 * Items in the source array and this array should have the same data structure.
	 * 
	 * @param[in]  item   starting pointer of another array
	 * @param[in]  num    number of items to copy
	 * @return            true - success; false - fail
	 */
	bool   push(void * item, int num);

	/**
	 * @return  pointer to the next free item
	 */
	void * next();

	/**
	* @brief  rollback to prev node
	*/
	void rollback();

	/**
	 * @return  first address of the array
	 */
	inline void  *head(){
		return m_data;
	}

	/**
	 * @return  pointer to the index-th item
	 */
	inline void *at(int index){
		return ((char *)m_data + m_elem_size * index);
	}

	/**
	 * @return  number of items in the array
	 */
	inline unsigned int  size(int count = 0){
		if (count > 0) {
			m_use_count = count;
		}
		return m_use_count;
	}

	void   clear();
	void   reset();

	/**
	 * Expand the array to a new capacity.
	 * If the current capacity is not less than the new capacity, nothing is done.
	 * 
	 * @param[in]  capacity   new capacity to expan
	 * @return                true - success; false - fail
	 */
	bool   expand(unsigned int capacity);

private:
	void          *m_data;
	char          *m_curr_pos;
	size_t         m_elem_size;
	unsigned int   m_use_count;
	unsigned int   m_total_count;
};

/*
   ----------------------------------------------
   |oooooooooooo|xxxxxxxxxxxxxxxxx|ooooooooooooo|
m_queue_data   m_front_pos    m_rear_pos     m_end_data
   ----------------------------------------------
*/

class RingQueue{

public:
	explicit RingQueue(int size, int count);
	~RingQueue();

	void  clear(); // Clear queue and data
	void  reset(); // Reuse data

	void * push(void *node);
	void * pop();

	void * next();     //get next node
	void * get_node(); //get one of available node

	void * at(int index){
		return ((char *)m_queue_data + m_elem_size * index);
	}

	inline bool is_empty(){
		return (m_use_num >= m_real_count);
	}

	inline bool is_full(){
		//keep 10% Buffer redundancy 
		return (m_real_count + m_max_count/10 >= m_use_num + m_max_count);
	}

	inline int64_t size(){
		return m_real_count - m_use_num;
	}

private:
	volatile int64_t m_use_num;
	volatile int64_t m_real_count;
	int   m_elem_size, m_max_count;
	char *m_front_pos, *m_rear_pos;
	void *m_queue_data, *m_end_data;
};


/**
* @class  SynQueue
* @brief  Applicable to the data in a multi threaded synchronization
*/

template<class TP>
class SynQueue
{
public:
	SynQueue(int max_count){
		m_storage = new RingQueue(sizeof(TP), max_count);
	}

	~SynQueue(){
		delete m_storage;
	}

	bool put(TP & item){
		std::lock_guard<std::mutex> locker(m_mutex);
		while (m_storage->is_full()){
			if (m_cond_full.wait_for(m_mutex, std::chrono::seconds(TIME_OUT))
				== std::cv_status::timeout){
				return false;
			}
		}
		m_storage->push((void *)&item);
		m_cond_empty.notify_one();
		return true;
	}

	bool get(TP *item){
		std::lock_guard<std::mutex> locker(m_mutex);
		while (m_storage->is_empty()){
			if (m_cond_empty.wait_for(m_mutex, std::chrono::seconds(TIME_OUT))
				== std::cv_status::timeout){
				return false;
			}
		}
		void *temp = m_storage->pop();
		memcpy((void *)item, temp, sizeof(TP));
		m_cond_full.notify_one();
		return true;
	}

	inline bool is_empty(){
		return m_storage->is_empty();
	}

	inline void clear(){
		m_storage->clear();
	}

	inline void reset(){
		m_storage->reset();
	}

private:
	std::mutex    m_mutex;
	std::condition_variable_any m_cond_empty;
	std::condition_variable_any m_cond_full;
	RingQueue    *m_storage;
};

#endif
