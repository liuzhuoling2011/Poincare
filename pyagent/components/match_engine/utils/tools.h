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
#include "list.h"

#define  TIME_OUT   500

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


/**
* @class  MyHash
*
* @brief  Provide a general hash table.
*         Support the (string->void*) mapping relationship
*/
struct hash_base{
	char        str_key[64];
	list_head   hs_link;
	//char      my_data[0];
};

class HashTable{

public:
	HashTable(size_t size, int count);
	~HashTable();
	void clear();

	void* create();
	void  insert(void *item, bool cover=false);

	void* find(int num);
	void* find(const char* str_key);

	uint64_t hash_value(const char *str_key);
	inline int size(){
		return m_use_count;
	}

public:
	size_t         m_elem_size;
	int            m_use_count;
	int            m_total_count;
	int            m_table_size;

	struct hash_bucket {
		list_head     head;
	};
	hash_bucket   *m_bucket;
	MyArray       *m_data;
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
