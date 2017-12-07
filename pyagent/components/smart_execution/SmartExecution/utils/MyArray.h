#ifndef __MYARRAY_H__
#define __MYARRAY_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
* @class   MyArray
*
* @brief   High performance dynamic array to store POD structs.
*          We believe MyArray provides an unparalleled combination of performance and memory usage.
*
* @notice  1. Tell MyArray max size you need in constructor to avoid memory reallocated
*          2. Recommand to use get_next_free_node() to get full performace
*          3. We should use insert && erase as little as possible, it will spent much time to move memory.
*          4. Don't point to item in MyArray, every reallocate may make pointers invalid.
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

class MyArray {

public:
	/**
	* Create an array to contain items. Preallocate space enough for `count` items.
	*
	* @param[in]  count  initial capacity
	*/
	MyArray(size_t count = 64) {
		m_elem_size = sizeof(T);
		m_data = (T*)calloc(count, m_elem_size);
		m_use_count = 0;
		m_total_count = count;
		m_last_pos = m_data;
	}

	void operator= (const MyArray<T> &other) {
		m_elem_size = other.m_elem_size;
		m_use_count = other.m_use_count;
		m_total_count = other.m_total_count;

		if (m_data != NULL) {
			free(m_data);
			m_data = (T*)calloc(m_total_count, m_elem_size);
			memcpy(m_data, other.m_data, m_use_count * m_elem_size);
		}

		m_last_pos = m_data + m_use_count;
	}

	T& operator[] (size_t pos) {
		return *at(pos);
	}

	~MyArray() {
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
		T* new_data = (T*)calloc(count, m_elem_size);
		if (m_use_count > count) {
			m_use_count = count;
		}
		memcpy(new_data, m_data, m_use_count * m_elem_size);
		free(m_data);
		m_data = new_data;

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
	T& get_next_free_node() {
		if (m_use_count >= m_total_count) {
			resize(m_total_count << 1);
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
	void push_back(const T& item) {
		if (m_use_count == m_total_count) {
			resize(m_total_count << 1);
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
			size_t need = m_total_count << 1;
			while (need < m_use_count + count) {
				need = need << 1;
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
	void insert(size_t index, const T& item) {
		if (index > m_total_count) return;

		if (m_total_count == m_use_count) {
			resize(m_total_count << 1);
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
			size_t need = m_total_count << 1;
			while (need < m_use_count + count) {
				need = need << 1;
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

#endif