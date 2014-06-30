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
#include <IGeometry.h>
#include <IInlineGraphic.h>
#include <IItemStrand.h>
#include <ITextModel.h>
#include <IWaxGlyphs.h>
#include <IWaxRun.h>
// Library headers
#include <textiterator.h>
// Module header
#include "InlineObjectRun.h"
//#include "Line.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations
using namespace nrsc;


//run * inline_object::clone_empty() const
//{
//	return new inline_object();
//}


bool inline_object::layout_span(cluster_thread & thread, TextIterator ti, size_t span)
{
	InterfacePtr<const ITextModel> model(ti.QueryTextModel());
	if (model == nil)
		return false;

	InterfacePtr<IItemStrand> strand(static_cast<IItemStrand*>(model->QueryStrand(kOwnedItemStrandBoss, IItemStrand::kDefaultIID)));
	if (strand == nil)
		return false;

	// A missing asset is no reason to stop composition.
	UID inline_UID = strand->GetOwnedUID(ti.Position(), kInlineBoss);
	if (inline_UID == kInvalidUID)
		return true;

	// The Glyph run's dimensions match those of the inline.
	InterfacePtr<IGeometry> graphic(::GetDataBase(model), inline_UID, UseDefaultIID());
	if (graphic == nil)
		return false;

	const_cast<ops *>(static_cast<const ops *>(_ops))->inline_UID_ref = ::GetUIDRef(graphic);
	PMRect inline_bounding_box = graphic->GetStrokeBoundingBox();

	// Inline geometry is relative to the baseline of the text. Don't want to add stretch
	height() = -inline_bounding_box.Top();
	cluster cl;
	cl.add_glyf(glyf(kInvalidGlyphID, glyf::fixed, inline_bounding_box.Width()));
	cl.add_chars();
	thread.push_back(cl);

	return true;
}


bool inline_object::ops::render(cluster_thread::run const & run, IWaxGlyphs & glyphs) const
{
	// Check we have an inline buffered and it's valid;
	if (run.size() != 1 || inline_UID_ref == UIDRef(nil, kInvalidUID))	
		return false;

	InterfacePtr<IInlineGraphic> graphic(&glyphs, UseDefaultIID());
	if (graphic == nil)	return false;

	graphic->SetGraphic(inline_UID_ref);

	glyphs.AddGlyph(run.front().front().id(), ToFloat(run.front().width()));

	return true;
}


void inline_object::ops::destroy()
{
	delete this;
}


const cluster_thread::run::ops * inline_object::ops::clone() const
{
	return new ops(*this);
}

