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
#include "IWaxGlyphsME.h"
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



cluster_thread::_span::size_type cluster_thread::_span::num_glyphs() const
{
	size_type n = 0;
	for (const_iterator cl_i = begin(); cl_i != end(); ++cl_i)
		n += cl_i->size();

	return n;
}

cluster_thread::_span::size_type cluster_thread::_span::span() const
{
	size_type n = 0;
	for (const_iterator cl_i = begin(); cl_i != end(); ++cl_i)
		n += cl_i->span();

	return n;
}


cluster_thread::run::~run()
{
	const_cast<ops *>(_ops)->destroy();
}


void cluster_thread::run::apply_desired_widths()
{
	InterfacePtr<IJustificationStyle>	js(get_style(), UseDefaultIID());
	const PMReal	space_width = get_style()->GetSpaceWidth();

	adjust_widths(0, js->GetAlteredWordspace()-space_width, js->GetAlteredLetterspace(false), 0);
}


void cluster_thread::run::adjust_widths(PMReal fill_space, PMReal word_space, PMReal letter_space, PMReal glyph_scale)
{
	for (iterator cl = begin(), cl_e = end(); cl != cl_e; ++cl)
	{
		for (cluster::iterator g = cl->begin(), g_e = cl->end(); g != g_e; ++g)
		{
			g->kern(glyph_scale*g->width());
			g->shift(glyph_scale*g->pos().X());

			switch (g->justification())
			{
			case glyf::fill:
				g->kern(fill_space);
				break;
			case glyf::space:
				g->kern(letter_space + word_space);
				break;
			case glyf::letter:
				g->kern(letter_space);
				break;
			case glyf::glyph:
				g->shift(-letter_space);
				break;
			case glyf::fixed:
				if (g->width() > 0) g->kern(letter_space);
				break;
			default: 
				break;
			}
		}
	}

	extra_scale() = glyph_scale;
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


void cluster_thread::run::calculate_stretch(glyf::stretch const & js, glyf::stretch & s) const
{
	const PMReal	space_width = _ds->GetSpaceWidth();

	for (const_iterator cl = begin(), cl_e = end(); cl != cl_e; ++cl)
		cl->calculate_stretch(space_width, js, s);
}


void cluster_thread::get_stretch_ratios(glyf::stretch & s) const
{
	PMReal min,des,max;
	InterfacePtr<IJustificationStyle>	js(runs().front().get_style(), UseDefaultIID());

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

	s[glyf::fill].min = s[glyf::space].min;
	s[glyf::fill].max = 1000000.0;
	s[glyf::fill].num = 0;
}


void cluster_thread::collect_stretch(glyf::stretch & s) const
{
	glyf::stretch js = {{0,0},{0,0},{0,0},{0,0},{0,0}};
	memset(&s, 0, sizeof(glyf::stretch));

	get_stretch_ratios(js);

	// We only want to consider stretch up to trailing whitespace
	for (runs_t::const_iterator r = runs().begin(), r_e = trailing_whitespace().run_iter(); r != r_e; ++r)
		r->calculate_stretch(js, s);

	// ... unless the whitespace is a fill character.
	for (const_iterator cl = trailing_whitespace(), cl_e = end(); cl != cl_e; ++cl)
	{
		if (cl->front().justification() != glyf::fill) continue;

		PMReal const space_width = cl.run().get_style()->GetSpaceWidth();
		s[glyf::fill].min += space_width*js[glyf::fill].min;
		s[glyf::fill].max += space_width*js[glyf::fill].max;
		++s[glyf::fill].num;
	}
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

	if (!_runs.empty() && base_t::end() == _runs.back().end())
		_runs.back()._e = cl;
}



cluster_thread::run::ops	cluster_thread::run::_default_ops;

bool cluster_thread::run::ops::render(const nrsc::cluster_thread::run &run, IWaxGlyphs &glyphs) const
{
	const size_t num = run.num_glyphs();
	float * x_offs = new float[num],
		  * y_offs = new float[num],
		  * widths = new float[num];

	//Assemble glyphs
	float * xo = x_offs,
		  * yo = y_offs,
		  * w  = widths;
	for (const_iterator cl_i = run.begin(); cl_i != run.end(); ++cl_i)
	{
		const cluster & cl = *cl_i;
		for (cluster::const_iterator g = cl.begin(); g != cl.end(); ++g, ++w, ++xo, ++yo)
		{
			*w = 0;
			*xo = ToFloat(g->pos().X());
			*yo = ToFloat(g->pos().Y());
			glyphs.AddGlyph(g->id(), ToFloat(g->advance()));
		}
	}

	// Position shifted glyphs
	InterfacePtr<IWaxGlyphsME> glyphs_me(&glyphs, UseDefaultIID());
	if (glyphs_me != nil)
		glyphs_me->AddGlyphMEData(num, x_offs, y_offs, widths);

	// Do the mappings
	int	i  = 0, 
		gi = 0;
	for (const_iterator cl_i = run.begin(); cl_i != run.end(); ++cl_i)
	{
		const cluster & cl = *cl_i;
		
		glyphs.AddMappingWidth(cl.width());
		glyphs.AddMappingRange(i++, gi, cl.size());
		for (unsigned char n = cl.span()-1; n; --n, ++i)
		{
			glyphs.AddMappingWidth(0);
			glyphs.AddMappingRange(i, gi, cl.size());
		}
		gi += cl.size();
	}

	delete [] x_offs;
	delete [] y_offs;
	delete [] widths;

	return true;
}


void cluster_thread::run::ops::destroy()
{
}


const cluster_thread::run::ops * cluster_thread::run::ops::clone() const
{
	return this;
}