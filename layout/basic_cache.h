/*
The MIT License (MIT)

Copyright (c) 2013 SIL International

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#pragma once

// Language headers
#include <list>
// Interface headers
// Library headers
// Module header

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations


namespace nrsc
{

	/** A simple cache managment base class, implement an LRO policy.
	Lifetime is recorded counting accesses and an entry's dwell time is reset
	if it is accessed and an entry is evicted if it is the oldest and the
	cache is at capacity or if it's dwell time is greater than the maximum
	allowed.
	*/

template<typename K, typename V, typename Kr=K>
class basic_cache
{
public:
	typedef K		key_t;
	typedef V		value_t;

	/** Create a basic_cache.
		@param capacity IN The maximum number of values that should be 
			cached at any time. A value of 0 signifies unlimited capacity
		@param max_dwell IN The maimum dwell time. A value of 0 signifies 
			unlimited time.
		
	*/
    basic_cache(size_t capacity, unsigned int max_dwell=0)
	: _capacity(capacity), 
	  _max_dwell(max_dwell)
	{
	}

	virtual ~basic_cache() throw()
	{
		destroy_entries();
	}

	bool empty() const 
	{
		return _faces.empty();
	}

	size_t size() const
	{
		return _faces.size();
	}

	size_t capacity() const
	{
		return _capacity;
	}

	value_t	operator [] (const key_t k)
	{
		key_ref_t kref = resolve_key(k);

		store_t::iterator i = std::find(_entries.begin(), _entries.end(), kref);
		if (i == _entries.end())
		{
			spring_clean();
			i = _entries.insert(_entries.begin(), make_entry(kref));
		}

		return i == _entries.end() ? 0 : freshen(i).value;
	}


protected:
	typedef Kr		key_ref_t;
	struct entry
	{
		key_ref_t	key;
		value_t		value;

		entry(key_ref_t & k, value_t v) throw() : key(k), value(v)	{}

		bool operator == (const key_ref_t & rhs) const throw()	{ return rhs == key; }	
	};

	virtual key_ref_t	resolve_key(const key_t &) const = 0;
	virtual entry		make_entry(key_ref_t &) const = 0;
	virtual void		destroy_entry(entry &) throw() = 0;

private:
	typedef std::list<entry>	store_t;

	void destroy_entries() throw()
	{
		for (store_t::iterator i = _entries.begin(), e = _entries.end(); i != e; ++i)
			destroy_entry(*i);
	}

	void spring_clean()
	{
		while (_entries.size() >= _capacity)
		{
			destroy_entry(_entries.back());
			_entries.pop_back();
		}
	}

	const entry & freshen(const typename store_t::iterator & i)
	{
		_entries.splice(_entries.begin(), _entries, i); 
		return _entries.front();
	}

	store_t					_entries;
	const size_t			_capacity;
	const unsigned int		_max_dwell;
};


} // end of namespace nrsc