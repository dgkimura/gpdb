//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLWindowKey.h
//
//	@doc:
//		Class for representing DXL window key
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLWindowKey_H
#define GPDXL_CDXLWindowKey_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLWindowFrame.h"

namespace gpdxl
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CDXLWindowKey
//
//	@doc:
//		Class for representing DXL window key
//
//---------------------------------------------------------------------------
class CDXLWindowKey : public CRefCount
{
private:
	// window frame associated with the window key
	gpos::Ref<CDXLWindowFrame> m_window_frame_dxl{nullptr};

	// sorting columns
	gpos::Ref<CDXLNode> m_sort_col_list_dxlnode{nullptr};

public:
	CDXLWindowKey(const CDXLWindowKey &) = delete;

	// ctor
	CDXLWindowKey();

	// dtor
	~CDXLWindowKey() override;

	// serialize operator in DXL format
	virtual void SerializeToDXL(CXMLSerializer *) const;

	// set window frame definition
	void SetWindowFrame(gpos::Ref<CDXLWindowFrame> window_frame);

	// return window frame
	gpos::Ref<CDXLWindowFrame>
	GetWindowFrame() const
	{
		return m_window_frame_dxl.get();
	}

	// set the list of sort columns
	void SetSortColList(gpos::Ref<CDXLNode> sort_col_list_dxlnode);

	// sort columns
	gpos::Ref<CDXLNode>
	GetSortColListDXL() const
	{
		return m_sort_col_list_dxlnode.get();
	}
};

using CDXLWindowKeyArray = gpos::Vector<gpos::Ref<CDXLWindowKey>>;
}  // namespace gpdxl
#endif	// !GPDXL_CDXLWindowKey_H

// EOF
