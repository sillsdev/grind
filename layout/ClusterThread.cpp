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

// Language headers
// Interface headers
#include "VCPlugInHeaders.h"
// Library headers
// Module header
#include "ClusterThread.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations

using namespace nrsc;

inline
cluster_thread::run cluster_thread::run::split(cluster_thread::base_t::iterator const & i)
{
	run r(i,_e);

	_s -= r._s; 
	_e = i; 
	
	return r;
}


cluster_thread cluster_thread::split(iterator const & i)
{
	cluster_thread res;

	// Split this cluster's run and push the rhs onto the result.
	res._runs.push_back(i.run().split(i._cl));
	if (res._runs.back().empty())	res.pop_back();

	// Splice the runs after the cluster's run into the result
	res._runs.splice(res._runs.end(), _runs, ++runs_t::iterator(i._r), _runs.end());
	if (_runs.back().empty()) _runs.pop_back();

	// Splice the clusters starting at i till the end into the result.
	res.splice(res.base_t::end(), *this, i._cl, base_t::end());

	return res;
}
