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
// Interface headers
// Library headers
// Module header
#include "Run.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
struct gr_face;

namespace nrsc 
{
// Project forward declarations

class graphite_run : public run
{
	const gr_face * const	_face;

//	graphite_run();

	// Prevent automatic copy-ctor and assignment generation
	//graphite_run(const graphite_run &);
	//graphite_run & operator = (const graphite_run &);

	virtual bool layout_span(cluster_thread & thread, TextIterator first, size_t span);
//	virtual run * clone_empty() const;

public:
	graphite_run(const gr_face * const, IDrawingStyle * ds);
};


//inline
//graphite_run::graphite_run()
//: _face(0)
//{
//}

inline
graphite_run::graphite_run(const gr_face * const face, IDrawingStyle * ds)
: run(ds),
  _face(face)
{
}

} // end of namespace nrsc