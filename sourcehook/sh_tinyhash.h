/* ======== SourceMM ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): David "BAILOPAN" Anderson
* ============================
*/

#ifndef _INCLUDE_SH_TINYHASH_H_
#define _INCLUDE_SH_TINYHASH_H_

#include "sh_list.h"

#define _T_INIT_HASH_SIZE	32

namespace SourceHook
{
	template <class K>
	int HashFunction(const K & k);

	template <class K>
	int Compare(const K & k1, const K & k2);

	/**
	 * This is a tiny, growable hash class.
	 * Meant for quick and dirty dictionaries only!
	 */
	template <class K, class V> 
	class THash
	{
	public:
		struct THashNode
		{
			THashNode(const K & k, const V & v) : 
				key(k), val(v)
				{
				};
			K key;
			V val;
		};
		typedef List<THashNode *> *	NodePtr;
	public:
		THash() : m_numBuckets(0), m_Buckets(NULL)
		{
			_Refactor();
		}
		size_t GetBuckets()
		{
			return m_numBuckets;
		}
		float PercentUsed()
		{
			return m_percentUsed;
		}
		V & operator [](const K & key)
		{
			THashNode *pNode = _FindOrInsert(key);
			return pNode->val;
		}
	private:
		THashNode *_FindOrInsert(const K & key)
		{
			size_t place = HashFunction(key) % m_numBuckets;
			THashNode *pNode = NULL;
			if (!m_Buckets[place])
			{
				m_Buckets[place] = new List<THashNode *>;
				pNode = new THashNode(key, V());
				m_Buckets[place]->push_back(pNode);
				m_percentUsed += (1.0f / (float)m_numBuckets);
			} else {
				List<THashNode *>::iterator iter;
				for (iter=m_Buckets[place]->begin(); iter!=m_Buckets[place]->end(); iter++)
				{
					if (Compare((*iter)->key, key) == 0)
						return (*iter);
				}
				//node does not exist
				pNode = new THashNode(key, V());
				m_Buckets[place]->push_back(pNode);
			}
			if (PercentUsed() > 0.75f)
				_Refactor();
			return pNode;
		}
		void _Refactor()
		{
			m_percentUsed = 0.0f;
			if (!m_numBuckets)
			{
				m_numBuckets = _T_INIT_HASH_SIZE;
				m_Buckets = new NodePtr[m_numBuckets];
				for (size_t i=0; i<m_numBuckets; i++)
					m_Buckets[i] = NULL;
			} else {
				size_t oldSize = m_numBuckets;
				m_numBuckets *= 2;
				List<THashNode *>::iterator iter;
				size_t place;
				THashNode *pHashNode;
                NodePtr *temp = new NodePtr[m_numBuckets];
				for (size_t i=0; i<m_numBuckets; i++)
					temp[i] = NULL;
				//look in old hash table
				for (size_t i=0; i<oldSize; i++)
				{
					//does a bucket have anything?
					if (m_Buckets[i])
					{
						//go through the list of items
						for (iter = m_Buckets[i]->begin(); iter != m_Buckets[i]->end(); iter++)
						{
							pHashNode = (*iter);
							//rehash it with the new bucket filter
							place = HashFunction(pHashNode->key) % m_numBuckets;
							//add it to the new hash table
							if (!temp[place])
							{
								temp[place] = new List<THashNode *>;
								m_percentUsed += (1.0f / (float)m_numBuckets);
							}
							temp[place]->push_back(pHashNode);
						}
						//delete that bucket!
						delete m_Buckets[i];
						m_Buckets[i] = NULL;
					}
				}
				//reassign bucket table
				delete [] m_Buckets;
				m_Buckets = temp;
			}
		}
	public:
		friend class iterator;
		class iterator
		{
			friend class THash;
		public:
			iterator() : hash(NULL), end(true)
			{
			};
			iterator(THash *h) : hash(h), end(false)
			{
				if (!h->m_Buckets)
					end = true;
				else
					_Inc();
			};
			//pre increment
			iterator & operator++()
			{
				_Inc();
				return *this;
			}
			//post increment
			iterator operator++(int)
			{
				iterator old(*this);
				_Inc();
				return old;
			}
			THashNode & operator * () const
			{
				return *(*iter);
			}
			THashNode & operator * ()
			{
				return *(*iter);
			}
			THashNode * operator ->() const
			{
				return (*iter);
			}
			THashNode * operator ->()
			{
				return (*iter);
			}
			bool operator ==(const iterator &where) const
			{
				if (where.hash == this->hash
					&& where.end == this->end
					&& 
					 (this->end || 
					   ((where.curbucket == this->curbucket) 
					     && (where.iter == iter))
						 ))
					return true;
				return false;
			}
			bool operator !=(const iterator &where) const
			{
				return !( (*this) == where );
			}
		private:
			void _Inc()
			{
				if (end || !hash)
					return;
				if (curbucket < 0)
				{
					for (int i=0; i<(int)hash->m_numBuckets; i++)
					{
						if (hash->m_Buckets[i])
						{
							iter = hash->m_Buckets[i]->begin();
							if (iter == hash->m_Buckets[i]->end())
								continue;
							curbucket = i;
							break;
						}
					}
					if (curbucket < 0)
						end = true;
				} else {
					if (iter != hash->m_Buckets[curbucket]->end())
						iter++;
					if (iter == hash->m_Buckets[curbucket]->end())
					{
						int oldbucket = curbucket;
						for (int i=curbucket+1; i<(int)hash->m_numBuckets; i++)
						{
							if (hash->m_Buckets[i])
							{
								iter = hash->m_Buckets[i]->begin();
								if (iter == hash->m_Buckets[i]->end())
									continue;
								curbucket = i;
								break;
							}
						}
						if (curbucket == oldbucket)
							end = true;
					}
				}
			}
		private:
			int curbucket;
			typename SourceHook::List<THashNode *>::iterator iter;
			THash *hash;
			bool end;
		};
	public:
		iterator begin()
		{
			return iterator(this);
		}
		iterator end()
		{
			iterator iter;
			iter.hash = this;
			return iter;
		}
		template <typename U>
		iterator find(const U & u) const
		{
			iterator b = begin();
			iterator e = end();
			for (iterator iter = b; iter != e; iter++)
			{
				if ( (*iter).key == u )
					return iter;
			}
			return end();
		}
		template <typename U>
		iterator find(const U & u)
		{
			iterator b = begin();
			iterator e = end();
			for (iterator iter = b; iter != e; iter++)
			{
				if ( (*iter).key == u )
					return iter;
			}
			return end();
		}
	private:
		NodePtr	*m_Buckets;
		size_t m_numBuckets;
		float m_percentUsed;
	};
};

#endif //_INCLUDE_SH_TINYHASH_H_
