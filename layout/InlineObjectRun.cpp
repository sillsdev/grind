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


run * inline_object::clone_empty() const
{
	return new inline_object(_drawing_style);
}


bool inline_object::layout_span(TextIterator ti, size_t span)
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

	_inline_UID_ref = ::GetUIDRef(graphic);
	PMRect inline_bounding_box = graphic->GetStrokeBoundingBox();

	// Inline geometry is relative to the baseline of the text. Don't want to add stretch
	cluster * cl = new cluster();
	cl->add_glyf(glyf(kInvalidGlyphID, glyf::fixed, inline_bounding_box.Width(), -inline_bounding_box.Top(), 0));
	cl->add_chars();
	push_back(cl);

	return true;
}


bool inline_object::render_run(IWaxRun & run) const
{
	// Check we have an inline buffered and it's valid;
	if (size() != 1 || _inline_UID_ref == UIDRef(nil, kInvalidUID))	
		return false;

	InterfacePtr<IInlineGraphic> graphic(&run, UseDefaultIID());
	if (graphic == nil)	return false;

	graphic->SetGraphic(_inline_UID_ref);

	InterfacePtr<IWaxGlyphs> glyphs(&run, UseDefaultIID());
	if (glyphs == nil)	return false;

	glyphs->AddGlyph(front()->front().id(), ToFloat(front()->width()));

	return true;
}
