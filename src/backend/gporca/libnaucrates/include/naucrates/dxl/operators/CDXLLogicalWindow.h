//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalWindow.h
//
//	@doc:
//		Class for representing DXL logical window operators
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLLogicalWindow_H
#define GPDXL_CDXLLogicalWindow_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLLogical.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLWindowSpec.h"

namespace gpdxl
{
//---------------------------------------------------------------------------
//	@class:
//		CDXLLogicalWindow
//
//	@doc:
//		Class for representing DXL logical window operators
//
//---------------------------------------------------------------------------
class CDXLLogicalWindow : public CDXLLogical
{
private:
	// array of window specifications
	gpos::owner<CDXLWindowSpecArray *> m_window_spec_array;

	// private copy ctor
	CDXLLogicalWindow(CDXLLogicalWindow &);

public:
	//ctor
	CDXLLogicalWindow(CMemoryPool *mp,
					  gpos::owner<CDXLWindowSpecArray *> pdrgpdxlwinspec);

	//dtor
	~CDXLLogicalWindow() override;

	// accessors
	Edxlopid GetDXLOperator() const override;
	const CWStringConst *GetOpNameStr() const override;

	// number of window specs
	ULONG NumOfWindowSpecs() const;

	// return the window key at a given position
	gpos::pointer<CDXLWindowSpec *> GetWindowKeyAt(ULONG idx) const;

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						gpos::pointer<const CDXLNode *> node) const override;

	// conversion function
	static gpos::cast_func<CDXLLogicalWindow *>
	Cast(CDXLOperator *dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopLogicalWindow == dxl_op->GetDXLOperator());

		return dynamic_cast<CDXLLogicalWindow *>(dxl_op);
	}

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(gpos::pointer<const CDXLNode *>,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG
};
}  // namespace gpdxl
#endif	// !GPDXL_CDXLLogicalWindow_H

// EOF
