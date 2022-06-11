//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLWindowSpec.h
//
//	@doc:
//		Class for representing DXL window specification in the DXL
//		representation of the logical query tree
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLWindowSpec_H
#define GPDXL_CDXLWindowSpec_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLWindowFrame.h"
#include "naucrates/md/CMDName.h"

namespace gpdxl
{
using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@class:
//		CDXLWindowSpec
//
//	@doc:
//		Class for representing DXL window specification in the DXL
//		representation of the logical query tree
//
//---------------------------------------------------------------------------
class CDXLWindowSpec : public CRefCount
{
private:
	// memory pool;
	CMemoryPool *m_mp;

	// partition-by column identifiers
	gpos::owner<ULongPtrArray *> m_partition_by_colid_array;

	// name of window specification
	CMDName *m_mdname;

	// sorting columns
	gpos::owner<CDXLNode *> m_sort_col_list_dxlnode;

	// window frame associated with the window key
	gpos::owner<CDXLWindowFrame *> m_window_frame;

	// private copy ctor
	CDXLWindowSpec(const CDXLWindowSpec &);

public:
	// ctor
	CDXLWindowSpec(CMemoryPool *mp, ULongPtrArray *partition_by_colid_array,
				   CMDName *mdname, CDXLNode *sort_col_list_dxlnode,
				   CDXLWindowFrame *window_frame);

	// dtor
	~CDXLWindowSpec() override;

	// serialize operator in DXL format
	virtual void SerializeToDXL(CXMLSerializer *) const;

	// set window frame definition
	void SetWindowFrame(CDXLWindowFrame *window_frame);

	// return window frame
	gpos::pointer<CDXLWindowFrame *>
	GetWindowFrame() const
	{
		return m_window_frame;
	}

	// partition-by column identifiers
	gpos::pointer<ULongPtrArray *>
	GetPartitionByColIdArray() const
	{
		return m_partition_by_colid_array;
	}

	// sort columns
	gpos::pointer<CDXLNode *>
	GetSortColListDXL() const
	{
		return m_sort_col_list_dxlnode;
	}

	// window specification name
	CMDName *
	MdName() const
	{
		return m_mdname;
	}
};

using CDXLWindowSpecArray = CDynamicPtrArray<CDXLWindowSpec, CleanupRelease>;
}  // namespace gpdxl
#endif	// !GPDXL_CDXLWindowSpec_H

// EOF
