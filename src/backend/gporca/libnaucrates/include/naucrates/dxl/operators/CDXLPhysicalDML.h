//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalDML.h
//
//	@doc:
//		Class for representing physical DML operator
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalDML_H
#define GPDXL_CDXLPhysicalDML_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLDirectDispatchInfo.h"
#include "naucrates/dxl/operators/CDXLPhysical.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"

namespace gpdxl
{
enum EdxlDmlType
{
	Edxldmlinsert,
	Edxldmldelete,
	Edxldmlupdate,
	EdxldmlSentinel
};

//---------------------------------------------------------------------------
//	@class:
//		CDXLPhysicalDML
//
//	@doc:
//		Class for representing physical DML operators
//
//---------------------------------------------------------------------------
class CDXLPhysicalDML : public CDXLPhysical
{
private:
	// operator type
	const EdxlDmlType m_dxl_dml_type;

	// target table descriptor
	gpos::owner<CDXLTableDescr *> m_dxl_table_descr;

	// list of source column ids
	gpos::owner<ULongPtrArray *> m_src_colids_array;

	// action column id
	ULONG m_action_colid;

	// oid column id
	ULONG m_oid_colid;

	// ctid column id
	ULONG m_ctid_colid;

	// segmentid column id
	ULONG m_segid_colid;

	// should update preserve tuple oids
	BOOL m_preserve_oids;

	// tuple oid column id
	ULONG m_tuple_oid;

	// direct dispatch info for insert statements
	gpos::owner<CDXLDirectDispatchInfo *> m_direct_dispatch_info;

	// needs the data to be sorted or not
	BOOL m_input_sort_req;

public:
	CDXLPhysicalDML(const CDXLPhysicalDML &) = delete;

	// ctor
	CDXLPhysicalDML(
		CMemoryPool *mp, const EdxlDmlType dxl_dml_type,
		gpos::owner<CDXLTableDescr *> table_descr,
		gpos::owner<ULongPtrArray *> src_colids_array, ULONG action_colid,
		ULONG oid_colid, ULONG ctid_colid, ULONG segid_colid,
		BOOL preserve_oids, ULONG tuple_oid,
		gpos::owner<CDXLDirectDispatchInfo *> dxl_direct_dispatch_info,
		BOOL input_sort_req);

	// dtor
	~CDXLPhysicalDML() override;

	// operator type
	Edxlopid GetDXLOperator() const override;

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// DML operator type
	EdxlDmlType
	GetDmlOpType() const
	{
		return m_dxl_dml_type;
	}

	// target table descriptor
	gpos::pointer<CDXLTableDescr *>
	GetDXLTableDescr() const
	{
		return m_dxl_table_descr;
	}

	// source column ids
	gpos::pointer<ULongPtrArray *>
	GetSrcColIdsArray() const
	{
		return m_src_colids_array;
	}

	// action column id
	ULONG
	ActionColId() const
	{
		return m_action_colid;
	}

	// oid column id
	ULONG
	OidColId() const
	{
		return m_oid_colid;
	}

	// ctid column id
	ULONG
	GetCtIdColId() const
	{
		return m_ctid_colid;
	}

	// segmentid column id
	ULONG
	GetSegmentIdColId() const
	{
		return m_segid_colid;
	}

	// does update preserve oids
	BOOL
	IsOidsPreserved() const
	{
		return m_preserve_oids;
	}

	// tuple oid column id
	ULONG
	GetTupleOid() const
	{
		return m_tuple_oid;
	}

	// direct dispatch info
	gpos::pointer<CDXLDirectDispatchInfo *>
	GetDXLDirectDispatchInfo() const
	{
		return m_direct_dispatch_info;
	}

	// needs the data to be sorted or not
	BOOL
	IsInputSortReq() const
	{
		return m_input_sort_req;
	}

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(gpos::pointer<const CDXLNode *> node,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						gpos::pointer<const CDXLNode *> node) const override;

	// conversion function
	static gpos::cast_func<CDXLPhysicalDML *>
	Cast(CDXLOperator *dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopPhysicalDML == dxl_op->GetDXLOperator());

		return dynamic_cast<CDXLPhysicalDML *>(dxl_op);
	}
};
}  // namespace gpdxl

#endif	// !GPDXL_CDXLPhysicalDML_H

// EOF
