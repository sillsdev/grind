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
#include <ICompositionStyle.h>
#include <IDrawingStyle.h>
#include <IFontInstance.h>
#include <IJustificationStyle.h>
#include <IWaxGlyphs.h>
#include <IWaxGlyphsME.h>
#include <IWaxRenderData.h>
#include <IWaxRun.h>
// Library headers
#include <PMRealGlyphPoint.h>
#include <textiterator.h>
// Module header
#include "RunBuilder.h"
#include "ClusterThread.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations

using namespace nrsc;

const textchar kTextChar_EnQuadSpace = 0x2000;



run_builder::~run_builder()
{
}


bool run_builder::build_run(cluster_thread & thread, IDrawingStyle * ds, TextIterator & ti, TextIndex span)
{

	thread.open_run(ds, ds->GetLeading());
	_drawing_style = ds;
	_thread = &thread;
	_span = 0;

	InterfacePtr<IFontInstance>		font = ds->QueryFontInstance(kFalse);
	InterfacePtr<ICompositionStyle> comp_style(ds, UseDefaultIID());
	const PMReal	em_space_width = ds->GetEmSpaceWidth(false),
					space_width	= ds->GetSpaceWidth();
	if (font == nil || comp_style == nil)
		return false;

	TextIterator start = ti;


	for (; _span != span && !ti.IsNull(); ++ti, ++_span)
	{
		switch((*ti).GetValue())
		{
			case kTextChar_BreakRunInStyle:
				layout_span_with_spacing(start, ti, 0, glyf::space);
				++ti; ++_span;
				return true;
				break;
			case kTextChar_CR:
			case kTextChar_SoftCR:
				layout_span_with_spacing(start, ti, space_width, glyf::space);
				_thread->back().break_penalty() = cluster::penalty::mandatory;
				++ti; ++_span;
				return true; 
				break;
			case kTextChar_Tab:
				layout_span_with_spacing(start, ti, space_width, glyf::tab); 
				break;
			case kTextChar_Table:				
			case kTextChar_TableContinued:
			case kTextChar_ObjectReplacementCharacter:	
				layout_span(start, ti - start);
				_thread->back().break_penalty() = cluster::penalty::whitespace;
				return true; 
				break;
			case kTextChar_HardSpace:
				layout_span_with_spacing(start, ti, space_width, glyf::space);
				_thread->back().break_penalty() = cluster::penalty::never;
				break;
			case kTextChar_FlushSpace:
				layout_span_with_spacing(start, ti, space_width, glyf::fill); 
				break;
			case kTextChar_EnQuadSpace:
			case kTextChar_EnSpace:
				layout_span_with_spacing(start, ti, ds->GetEnSpaceWidth(false), glyf::fixed);
				break;
			case kTextChar_EmSpace:
				layout_span_with_spacing(start, ti, em_space_width, glyf::fixed);
				break;
			case kTextChar_ThirdSpace:
				layout_span_with_spacing(start, ti, em_space_width/3, glyf::fixed);
				break;
			case kTextChar_QuarterSpace:
				layout_span_with_spacing(start, ti, em_space_width/4, glyf::fixed);
				break;
			case kTextChar_SixthSpace:
				layout_span_with_spacing(start, ti, em_space_width/6, glyf::fixed);
				break;
			case kTextChar_FigureSpace:
				layout_span_with_spacing(start, ti, font->GetGlyphWidth(font->GetGlyphID(kTextChar_Zero)), glyf::fixed);
				break;
			case kTextChar_PunctuationSpace:
				layout_span_with_spacing(start, ti, font->GetGlyphWidth(font->GetGlyphID(kTextChar_Period)), glyf::fixed);
				break;
			case kTextChar_ThinSpace:
				layout_span_with_spacing(start, ti, em_space_width/8, glyf::fixed);
				break;
			case kTextChar_HairSpace:
				layout_span_with_spacing(start, ti, em_space_width/24, glyf::fixed);
				break;
			case kTextChar_NarrowNoBreakSpace:
				layout_span_with_spacing(start, ti, space_width, glyf::fixed);
				_thread->back().break_penalty() = cluster::penalty::never;
				break;
			case kTextChar_ZeroSpaceBreak:
				layout_span_with_spacing(start, ti, 0, glyf::fixed);
				break;
			case kTextChar_ZeroWidthNonJoiner:
			case kTextChar_ZeroWidthJoiner:
				layout_span_with_spacing(start, ti, 0, glyf::glyph);
				_thread->back().break_penalty() = cluster::penalty::never;
				break;
			case kTextChar_ZeroSpaceNoBreak:
				layout_span_with_spacing(start, ti, 0, glyf::fixed);
				_thread->back().break_penalty() = cluster::penalty::never;
				break;
			default: break;
		}
	}

	if (start != ti)
		layout_span(start, ti - start);

	_span += ti - start;

	assert(_span == _thread->runs().back().span());
	return true;
}


inline
void run_builder::layout_span_with_spacing(TextIterator & first, const TextIterator & last, PMReal width, glyf::justification_t level)
{
	const size_t span = last - first;
	if (span)
		layout_span(first, span);
		
	add_glue(level, width);
	first = last;
	++first;
	_span += span;
}


void run_builder::add_glue(glyf::justification_t level, PMReal width, cluster::penalty::type bw)
{
	cluster cl;

	cl.add_glyf(glyf(_drawing_style->GetSpaceGlyph(), level, width));
	cl.add_chars();
	cl.break_penalty() = bw;

	_thread->push_back(cl);
	++_span;
}


void run_builder::add_letter(int glyph_id, PMReal width, cluster::penalty::type bw, bool to_cluster)
{
	if (!to_cluster || _thread->runs().back().empty())
		_thread->push_back(cluster());

	cluster & cl = _thread->back();
	cl.add_glyf(glyf(glyph_id, to_cluster ? glyf::glyph : glyf::letter, width));
	cl.add_chars();
	cl.break_penalty() = bw;
	++_span;
}

