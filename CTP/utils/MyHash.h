#ifndef __MYHASH_H__
#define __MYHASH_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "utils/list.h"
#include "utils/utils.h"

/**
* @class   MyHash
*
* @brief   Provide a general hash table.
*          Support the (string->value) mapping relationship
*          We believe HashMap provides an unparalleled combination of performance and memory usage.
*
* @notice  1. Tell MyHash the max size you need in constructor to avoid memory reallocated
*          2. Recommand to use next_free_node() to avoid copy object twice
*          4. Don't point to item in MyHash, every reallocate may make pointers invalid.
*
* @author  liuzhuoling
* @date    20170918
*
*/

#define MAX_HASH_SIZE 64
#define HASH_KEY_SIZE 64
#define ROUND_UP(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

template <class V>
class MyHash {
public:
	struct HashNode {
		char key[HASH_KEY_SIZE];
		V    value;
		list_t hash_link;
		list_t free_link;
		list_t used_link;
	};

public:
	MyHash(size_t size = MAX_HASH_SIZE) {
		m_hash_size = ROUND_UP(size);
		m_data = (HashNode*)malloc(m_hash_size * sizeof(HashNode));
		m_hash_head = (list_t*)malloc(m_hash_size * sizeof(list_t));

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
		return l_node->value;
	}

	bool exist(const char* key) {
		HashNode *l_node = query(key);
		if (l_node == NULL)
			return false;
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

	V& get_next_free_node() {
		HashNode *l_node = get_free_node();
		m_cur_free_node = l_node;
		return l_node->value;
	}

	void insert_current_node(const char* key) {
		HashNode *l_node = query(key);
		if (l_node == NULL) {
			size_t hash_value = get_hash_value(key);
			list_add_after(&m_cur_free_node->hash_link, &m_hash_head[hash_value]);
			list_add_after(&m_cur_free_node->used_link, &m_used_head);

			strlcpy(m_cur_free_node->key, key, HASH_KEY_SIZE);
			m_use_count++;
			m_cur_free_node = NULL;
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
			insert_current_node(m_cur_free_node->key);
		}

		for (size_t i = m_hash_size; i < 2 * m_hash_size; i++) {
			INIT_LIST_HEAD(&m_data[i].hash_link);
			INIT_LIST_HEAD(&m_data[i].used_link);
			list_add_after(&m_data[i].free_link, &m_free_head);
		}

		m_hash_size = m_hash_size << 1;
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


#endif