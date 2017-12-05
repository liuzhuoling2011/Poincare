#ifndef __MYHASH_H__
#define __MYHASH_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "list.h"
#include "utils.h"

/**
* @class   MyHash
*
* @brief   Provide a general hash table.
*          Support the (string->value) mapping relationship
*          We believe HashMap provides an unparalleled combination of performance and memory usage.
*
* @notice  1. Tell MyHash the max size you need in constructor to avoid memory reallocated
*          2. Recommand to use get_next_free_node() to avoid copy object twice
*          4. Don't point to item in MyHash, every reallocate may make pointers invalid.
*
* @author  liuzhuoling
* @date    20170918
*
*/

#define MAX_HASH_SIZE 256
#define HASH_KEY_SIZE 64
#define ROUND_UP(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

template <class V>
class MyHash {
public:
	struct HashNode {
		char   first[HASH_KEY_SIZE];
		V      second;
		list_t hash_link;
		list_t free_link;
		list_t used_link;
	};
	struct Iterator {
		void operator = (HashNode* node) {
			m_hash_node = node;
			m_cur_pos = &node->used_link;
		}
		bool operator != (Iterator& iter) {
			return m_cur_pos != iter.m_cur_pos;
		}
		void operator ++ (int) {  //(*this)++
			list_t* next = m_cur_pos->prev;
			HashNode* node = list_entry(next, HashNode, used_link);
			m_hash_node = node;
			m_cur_pos = next;
		}
		HashNode* operator->() {
			return m_hash_node;
		}
		HashNode *m_hash_node;
		list_t *m_cur_pos = NULL;
	};

public:
	MyHash(size_t size = MAX_HASH_SIZE) {
		m_hash_size = ROUND_UP(size);
		m_data = (HashNode*)calloc(m_hash_size, sizeof(HashNode));
		m_hash_head = (list_t*)calloc(m_hash_size, sizeof(list_t));

		clear();
	}

	~MyHash() {
		if (m_data != NULL) {
			free(m_data); m_data = NULL;
		}
		if (m_hash_head != NULL) {
			free(m_hash_head); m_hash_head = NULL;
		}
	}

	V& operator[] (const char* key) {
		HashNode *l_node = query(key);
		if (l_node != NULL) {
			return l_node->second;
		} else {
			V& freenode = get_next_free_node();
			insert_current_node(key);
			return freenode;
		}
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
		return l_node->second;
	}

	bool exist(const char* key) {
		HashNode *l_node = query(key);
		if (l_node == NULL)
			return false;
		return true;
	}

	Iterator& begin() {
		HashNode *l_node = list_entry(m_used_head.prev, HashNode, used_link);
		m_iterator = l_node;
		return m_iterator;
	}

	Iterator& end() {
		HashNode *l_node = list_entry(m_used_head.prev->next, HashNode, used_link);
		m_iterator = l_node;
		return m_iterator;
	}

	/*------------ Modifiers Related ------------*/
	void insert(const char* key, const V& value) {
		HashNode *l_node = query(key);
		if (l_node == NULL) {
			HashNode *l_node = get_free_node();

			size_t hash_value = get_hash_value(key);
			list_add_after(&l_node->hash_link, &m_hash_head[hash_value]);
			list_add_after(&l_node->used_link, &m_used_head);

			strlcpy(l_node->first, key, HASH_KEY_SIZE);
			l_node->second = value;

			m_use_count++;
		}
		else {
			l_node->second = value;
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

	V& get_next_free_node() {
		HashNode *l_node = get_free_node();
		m_cur_free_node = l_node;
		return l_node->second;
	}

	void insert_current_node(const char* key, bool expand_flag = false) {
		HashNode *l_node = query(key);
		if (l_node == NULL) {
			size_t hash_value = get_hash_value(key, expand_flag);
			list_add_after(&m_cur_free_node->hash_link, &m_hash_head[hash_value]);
			list_add_after(&m_cur_free_node->used_link, &m_used_head);

			strlcpy(m_cur_free_node->first, key, HASH_KEY_SIZE);
			m_use_count++;
		}
	}

private:
	size_t get_hash_value(const char* key, bool expand_flag = false) {
		if(expand_flag == true)
			return my_hash_value(key) & (2 * m_hash_size - 1);
		return my_hash_value(key) & (m_hash_size - 1);
	}

	void expand_size() {
		// Init hash, used, free list, and then insert previous node
		INIT_LIST_HEAD(&m_used_head);
		INIT_LIST_HEAD(&m_free_head);
		size_t new_hash_size = 2 * m_hash_size;
		m_use_count = 0;

		list_t* new_hash_head = (list_t*)calloc(new_hash_size, sizeof(list_t));
		free(m_hash_head);
		m_hash_head = new_hash_head;
		for (size_t i = 0; i < new_hash_size; i++) {
			INIT_LIST_HEAD(&m_hash_head[i]);
		}

		HashNode* new_data = (HashNode*)calloc(new_hash_size, sizeof(HashNode));
		memcpy(new_data, m_data, m_hash_size * sizeof(HashNode));
		free(m_data);
		m_data = new_data;

		for (size_t i = 0; i < m_hash_size; i++) {
			m_cur_free_node = &m_data[i];
			INIT_LIST_HEAD(&m_cur_free_node->free_link);
			insert_current_node(m_cur_free_node->first, true);
		}

		for (size_t i = m_hash_size; i < new_hash_size; i++) {
			INIT_LIST_HEAD(&m_data[i].hash_link);
			INIT_LIST_HEAD(&m_data[i].used_link);
			list_add_after(&m_data[i].free_link, &m_free_head);
		}

		m_hash_size = new_hash_size;
	}

	HashNode* query(const char* key) {
		struct list_head *pos, *n;
		HashNode *l_node;

		size_t hash_value = get_hash_value(key);

		list_for_each_safe(pos, n, &m_hash_head[hash_value]) {
			l_node = list_entry(pos, HashNode, hash_link);
			if (my_strcmp(l_node->first, key) == 0) {
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
	size_t  m_use_count = 0;
	size_t  m_hash_size = 0;
	list_t  m_free_head;
	list_t  m_used_head;
	list_t *m_hash_head = NULL;
	Iterator m_iterator;
	HashNode *m_cur_free_node = NULL; // uesd for high performance
	HashNode *m_data = NULL;
};


#endif