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
#include "IJustificationStyle.h"
#include "IWaxGlyphs.h"
#include "IWaxRenderData.h"
#include "IWaxRun.h"
// Library headers
// Module header
#include "ClusterThread.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations

using namespace nrsc;

inline
void cluster_thread::_span::split_into(base_t::iterator const & i, _span & rhs)
{
	if (i == _b || i == _e) return;

	rhs._b = i;
	rhs._e = _e;
	rhs._s = std::distance(i,_e);
	_e = rhs._b;
	_s -= rhs._s; 
}



size_t cluster_thread::_span::num_glyphs() const
{
	size_t n = 0;
	for (const_iterator cl_i = begin(); cl_i != end(); ++cl_i)
		n += cl_i->size();

	return n;
}


IWaxRun * cluster_thread::run::wax_run() const
{
	// Check we've got composed text to put in the run and a line to put it in.
	if (empty())	return nil;

	// Create a new wax run object.
	InterfacePtr<IWaxRun> wr(static_cast<IWaxRun*>(::CreateObject(kWaxTextRunBoss, IID_IWAXRUN)));
	if (wr == nil)	return nil;

	wr->SetYPosition(_ds->GetEffectiveBaseline());

	// Fill out the render data for the run from the drawing style.
	InterfacePtr<IWaxRenderData> rd(wr, UseDefaultIID());
	InterfacePtr<IWaxGlyphs>     glyphs(wr, UseDefaultIID());
	if (rd == nil || glyphs == nil)	return nil;

	_ds->FillOutRenderData(rd, kFalse/*horizontal*/);
	_ds->AddAdornments(wr);
	wr->AddRef();

	// Apply x skew.
	PMMatrix glyphs_mat;
	PMPoint pc;
	glyphs_mat = glyphs->GetAllGlyphsMatrix(&pc);
	glyphs_mat.SkewTo(_ds->GetSkewAngle());
	glyphs_mat.Scale(1+_extra_scale, 1.0);
	glyphs->SetAllGlyphsMatrix(glyphs_mat, pc);

	_ops->render(*this, *glyphs);

	return wr;
}


void cluster_thread::get_stretch_ratios(glyf::stretch & s) const
{
	PMReal min,des,max;
	InterfacePtr<IJustificationStyle>	js(_runs.front().get_style(), UseDefaultIID());

	s[glyf::fill].min = s[glyf::space].min;
	s[glyf::fill].max = 1000000.0;
	s[glyf::fill].num = 0;

	js->GetWordspace(&min, &des, &max);
	s[glyf::space].min = des - min;
	s[glyf::space].max = max - des;
	s[glyf::space].num = 0;

	js->GetLetterspace(&min, &des, &max);
	s[glyf::letter].min = des - min;
	s[glyf::letter].max = max - des;
	s[glyf::letter].num = 0;

	js->GetGlyphscale(&min, &des, &max);
	s[glyf::glyph].min = des - min;
	s[glyf::glyph].max = max - des;
	s[glyf::glyph].num = 0;

	s[glyf::fixed].min = 0;
	s[glyf::fixed].max = 0;
	s[glyf::fixed].num = 0;
}


cluster_thread cluster_thread::split(iterator const & i)
{
	cluster_thread res;

	// Split this cluster's run and push the rhs onto the result.
	res._runs.push_back(i.run());
	i.run().split_into(i, res._runs.back());
	if (res._runs.back().empty())	res._runs.pop_back();

	// Splice the runs after the cluster's run into the result
	res._runs.splice(res._runs.end(), _runs, ++runs_t::iterator(i._r), _runs.end());
	if (_runs.back().empty()) _runs.pop_back();

	// Splice the clusters starting at i till the end into the result.
	res.splice(res.base_t::end(), *this, i, base_t::end());

	return res;
}

void cluster_thread::trim_trailing_whitespace(const PMReal letter_space)
{
	InterfacePtr<IJustificationStyle>	js(_runs.front().get_style(), UseDefaultIID());

	if (_trailing_ws_run == _runs.end())
	{
		// Find the last non-whitespace cluster
		reverse_iterator	cl = rbegin();
		for (const reverse_iterator cl_e = rend(); cl != cl_e; ++cl)
			if (!cl->whitespace()) break;

		if (cl == rend())
		{
			_trailing_ws_run = _runs.begin();
			return;
		}
		_trailing_ws_run = cl.base()._r;
	}

	// Re-assign any letterspace contributed whitespace in the final
	// glyph to the adjacent trailing whitespace.
	if (_trailing_ws_run != _runs.end())
	{
		base_t::iterator non_ws = _trailing_ws_run->begin();
		--non_ws;
		non_ws->trim(letter_space);
		_trailing_ws_run->begin()->front().kern(letter_space);
	}
}


void cluster_thread::fit_trailing_whitespace(const PMReal margin)
{
	const size_t ws_count = std::distance(_trailing_ws_run->begin(), base_t::end());

	// Shrink the trailing whitespace to fit into the margin space
	const PMReal trailing_ws = margin/ws_count;
	for (base_t::iterator cl = _trailing_ws_run->begin(), cl_e = base_t::end(); cl != cl_e; ++cl)
		cl->front().kern(std::min(trailing_ws - cl->front().width(), PMReal(0)));
}


void cluster_thread::push_back(const value_type & val)
{
	base_t::iterator cl = insert(base_t::end(), val);
	runs_t::iterator lr = --_runs.end();

	if (lr->empty())
	{
		lr->_b = cl;
		(--lr)->_e = cl;
	}
	++lr->_s;
}

