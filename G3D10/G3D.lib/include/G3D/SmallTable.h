/**
\file G3D/SmallTable.h

Templated linear array backed table class. Very bad big O behavior. Prefer Table unless you have reason to believe this makes more sense

\maintainer Michael Mara, http://illuminationcodified.com
@created 2015-03-10
@edited  2015-03-10
Copyright 2000-2015, Morgan McGuire.
All rights reserved.
*/

#ifndef G3D_SmallTable_h
#define G3D_SmallTable_h

#include <cstddef>
#include "G3D/G3DString.h"

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/debug.h"
#include "G3D/System.h"
#include "G3D/g3dmath.h"
#include "G3D/EqualsTrait.h"
#include "G3D/HashTrait.h"
#include "G3D/MemoryManager.h"

#ifdef _MSC_VER
#   pragma warning (push)
// Debug name too long warning
#   pragma warning (disable : 4786)
#endif

namespace G3D {

	/**
	An unordered data structure mapping keys to values; maintained behind the scenes in a linear array

	\sa G3D::Table
	*/
	template<class Key, class Value, class HashFunc = HashTrait<Key>, class EqualsFunc = EqualsTrait<Key> >
	class SmallTable {
	public:

		/**
		The pairs returned by iterator.
		*/
		class Entry {
		public:
			Key    key;
			Value  value;
			Entry() {}
			Entry(const Key& k) : key(k) {}
			Entry(const Key& k, const Value& v) : key(k), value(v) {}
			bool operator==(const Entry &peer) const { return (key == peer.key && value == peer.value); }
			bool operator!=(const Entry &peer) const { return !operator==(peer); }
		};

	private:

		typedef SmallTable<Key, Value, HashFunc, EqualsFunc> ThisType;

		Array<Entry> m_data;

		shared_ptr<MemoryManager>  m_memoryManager;

	public:

		/**
		Creates an empty hash table using the default MemoryManager.
		*/
		SmallTable() { }


		/**
		Recommends that the table resize to anticipate at least this number of elements.
		*/
		void setSizeHint(size_t n) {
			if (n > m_data.size()) {
				m_data.reserve(n);
			}
		}

		/** Uses the default memory manager */
		SmallTable(const ThisType& h) {
			m_data.append(h.m_data);
		}


		SmallTable& operator=(const ThisType& h) {
			// No need to copy if the argument is this
			if (this != &h) {
				m_data.fastClear();
				m_data.append(h.m_data);
			}
			return *this;
		}

		/**
		C++ STL style iterator variable.  See begin().
		*/
		class Iterator {
		private:
			friend class SmallTable<Key, Value, HashFunc, EqualsFunc>;

			/**
				Index.
			*/
			size_t              index;

			bool                isDone;

			Array<Entry>*		dataPtr;

			/**
			Creates the end iterator.
			*/
			Iterator() : index(0), dataPtr(NULL) {
				isDone = true;
			}

			Iterator(Array<Entry>* data) :
				index(0),
				dataPtr(data) {

				if (isNull(data) || dataPtr->size() == 0) {
					// Empty table
					isDone = true;
					return;
				}


				index = 0;
				isDone = false;
			}

			/**
			If node is NULL, then finds the next element by searching through the bucket array.
			Sets isDone if no more nodes are available.
			*/
			void findNext() {
				++index;
				if (index >= dataPtr->size()) {
					index = 0;
					isDone = true;
					return;
				}
			}

		public:
			inline bool operator!=(const Iterator& other) const {
				return !(*this == other);
			}

			bool operator==(const Iterator& other) const {
				if (other.isDone || isDone) {
					// Common case; check against isDone.
					return (isDone == other.isDone);
				}
				else {
					return (index == other.index);
				}
			}

			/**
			Pre increment.
			*/
			Iterator& operator++() {
				debugAssert(!isDone);
				findNext();
				return *this;
			}

			/**
			Post increment (slower than preincrement).
			*/
			Iterator operator++(int) {
				Iterator old = *this;
				++(*this);
				return old;
			}

			const Entry& operator*() const {
				return (*dataPtr)[index];
			}

			const Value& value() const {
				return (*dataPtr)[index].value;
			}

			const Key& key() const {
				return (*dataPtr)[index].key;
			}

			Entry* operator->() const {
				debugAssert(notNull(dataPtr));
				return &((*dataPtr)[index]);
			}

			operator Entry*() const {
				debugAssert(notNull(dataPtr));
				return &((*dataPtr)[index]);
			}

			bool isValid() const {
				return !isDone;
			}

			/** @deprecated  Use isValid */
			bool hasMore() const {
				return !isDone;
			}
		};


		/**
		C++ STL style iterator method.  Returns the first Entry, which
		contains a key and value.  Use preincrement (++entry) to get to
		the next element.  Do not modify the table while iterating.
		*/
		Iterator begin() const {
			return Iterator(const_cast<Array<Entry>*>(&m_data));
		}

		/**
		C++ STL style iterator method.  Returns one after the last iterator
		element.
		*/
		const Iterator end() const {
			return Iterator();
		}

		/**
		Removes all elements. Guaranteed to free all memory associated with
		the table.
		*/
		void clear() {
			m_data.clear();
		}

		void fastClear() {
			m_data.fastClear();
		}


		/**
		Returns the number of keys.
		*/
		size_t size() const {
			return m_data.size();
		}


		/**
		If you insert a pointer into the key or value of a table, you are
		responsible for deallocating the object eventually.  Inserting
		key into a table is O(1), but may cause a potentially slow rehashing.
		*/
		void set(const Key& key, const Value& value) {
			getCreateEntry(key).value = value;
		}

	private:

		/** Helper for remove() and getRemove() */
		bool remove(const Key& key, Key& removedKey, Value& removedValue, bool updateRemoved) {
			for (int i = 0; i < m_data.size(); ++i) {
				if (m_data[i].key == key) {
					if (updateRemoved) {
						removedKey = m_data[i].key;
						removedValue = m_data[i].value;
					}
					m_data.remove(i);
					return true;
				}
			}
			return false;
		}

	public:

		/** If @a member is present, sets @a removed to the element
		being removed and returns true.  Otherwise returns false
		and does not write to @a removed. */
		bool getRemove(const Key& key, Key& removedKey, Value& removedValue) {
			return remove(key, removedKey, removedValue, true);
		}

		/**
		Removes an element from the table if it is present.
		@return true if the element was found and removed, otherwise  false
		*/
		bool remove(const Key& key) {
			Key x;
			Value v;
			return remove(key, x, v, false);
		}

	private:

		Entry* getEntryPointer(const Key& key) const {
			for (int i = 0; i < m_data.size(); ++i) {
				if (m_data[i].key == key) {
					return &m_data[i];
				}
			}

			return NULL;
		}

	public:

		/** If a value that is EqualsFunc to @a member is present, returns a pointer to the
		version stored in the data structure, otherwise returns NULL.
		*/
		const Key* getKeyPointer(const Key& key) const {
			const Entry* e = getEntryPointer(key);
			if (e == NULL) {
				return NULL;
			}
			else {
				return &(e->key);
			}
		}

		/**
		Returns the value associated with key.
		@deprecated Use get(key, val) or getPointer(key)
		*/
		Value& get(const Key& key) const {
			Entry* e = getEntryPointer(key);
			debugAssertM(e != NULL, "Key not found");
			return e->value;
		}


		/** Returns a pointer to the element if it exists, or NULL if it does not.
		Note that if your value type <i>is</i> a pointer, the return value is
		a pointer to a pointer.  Do not remove the element while holding this
		pointer.

		It is easy to accidentally mis-use this method.  Consider making
		a Table<Value*> and using get(key, val) instead, which makes you manage
		the memory for the values yourself and is less likely to result in
		pointer errors.
		*/
		Value* getPointer(const Key& key) const {
			for (int i = 0; i < m_data.size(); ++i) {
				if (m_data[i].key == key) {
					return const_cast<Value*>(&(m_data[i].value));
				}
			}

			return NULL;
		}

		/**
		If the key is present in the table, val is set to the associated value and returns true.
		If the key is not present, returns false.
		*/
		bool get(const Key& key, Value& val) const {
			Value* v = getPointer(key);
			if (v != NULL) {
				val = *v;
				return true;
			}
			else {
				return false;
			}
		}


		/** Called by getCreate() and set()

		\param created Set to true if the entry was created by this method.
		*/
		Entry& getCreateEntry(const Key& key, bool& created) {
			created = false;

			for (int i = 0; i < m_data.size(); ++i) {
				if (m_data[i].key == key) {
					return m_data[i];
				}
			}
			if (m_data.size() == 0) {
				m_data.reserve(30);
			}
			created = true;
			Entry& result = m_data.next();
			result.key = key;
			return result;
		}

		Entry& getCreateEntry(const Key& key) {
			bool ignore;
			return getCreateEntry(key, ignore);
		}


		/** Returns the current value that key maps to, creating it if necessary.*/
		Value& getCreate(const Key& key) {
			return getCreateEntry(key).value;
		}

		/** \param created True if the element was created. */
		Value& getCreate(const Key& key, bool& created) {
			return getCreateEntry(key, created).value;
		}


		/**
		Returns true if any key maps to value using operator==.
		*/
		bool containsValue(const Value& value) const {
			for (Iterator it = begin(); it.isValid(); ++it) {
				if (it.value == value) {
					return true;
				}
			}
			return false;
		}

		/**
		Returns true if key is in the table.
		*/
		bool containsKey(const Key& key) const {
			for (int i = 0; i < m_data.size(); ++i) {
				if (m_data[i].key == key) {
					return true;
				}
			}

			return false;
		}


		/**
		Short syntax for get.
		*/
		inline Value& operator[](const Key &key) const {
			return get(key);
		}

		/**
		Returns an array of all of the keys in the table.
		You can iterate over the keys to get the values.
		@deprecated
		*/
		Array<Key> getKeys() const {
			Array<Key> keyArray;
			getKeys(keyArray);
			return keyArray;
		}

		void getKeys(Array<Key>& keyArray) const {
			keyArray.resize(0, DONT_SHRINK_UNDERLYING_ARRAY);
			for (int i = 0; i < m_data.size(); ++i) {
				keyArray.append(m_data[i].key);
			}
		}

		/** Will contain duplicate values if they exist in the table.  This array is parallel to the one returned by getKeys() if the table has not been modified. */
		void getValues(Array<Value>& valueArray) const {
			valueArray.resize(0, DONT_SHRINK_UNDERLYING_ARRAY);
			for (int i = 0; i < m_data.size(); ++i) {
				valueArray.append(m_data[i].value);
			}
		}

		/**
		Calls delete on all of the keys and then clears the table.
		*/
		void deleteKeys() {
			for (int i = 0; i < m_data.size(); ++i) {
				delete m_data[i].key;
				m_data[i].key = NULL;
			}
			clear();
		}

		/**
		Calls delete on all of the values.  This is unsafe--
		do not call unless you know that each value appears
		at most once.

		Does not clear the table, so you are left with a table
		of NULL pointers.
		*/
		void deleteValues() {
			for (int i = 0; i < m_data.size(); ++i) {
				delete m_data[i].value;
				m_data[i].value = NULL;
			}
		}

		template<class H, class E>
		bool operator==(const Table<Key, Value, H, E>& other) const {
			if (size() != other.size()) {
				return false;
			}

			for (Iterator it = begin(); it.hasMore(); ++it) {
				const Value* v = other.getPointer(it->key);
				if ((v == NULL) || (*v != it->value)) {
					// Either the key did not exist or the value was not the same
					return false;
				}
			}

			// this and other have the same number of keys, so we don't
			// have to check for extra keys in other.

			return true;
		}

		template<class H, class E>
		bool operator!=(const Table<Key, Value, H, E>& other) const {
			return !(*this == other);
		}

	};

} // namespace

#ifdef _MSC_VER
#   pragma warning (pop)
#endif

#endif
