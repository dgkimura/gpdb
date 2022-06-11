//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CDXLLogicalCTAS.h
//
//	@doc:
//		Class for representing logical "CREATE TABLE AS" (CTAS) operator
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLLogicalCTAS_H
#define GPDXL_CDXLLogicalCTAS_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLColDescr.h"
#include "naucrates/dxl/operators/CDXLLogical.h"
#include "naucrates/md/IMDRelation.h"

namespace gpdxl
{
// fwd decl
class CDXLCtasStorageOptions;

using namespace gpmd;


//---------------------------------------------------------------------------
//	@class:
//		CDXLLogicalCTAS
//
//	@doc:
//		Class for representing logical "CREATE TABLE AS" (CTAS) operator
//
//---------------------------------------------------------------------------
class CDXLLogicalCTAS : public CDXLLogical
{
private:
	// mdid of table to create
	gpos::owner<IMDId *> m_mdid;

	// schema name
	CMDName *m_mdname_schema;

	// table name
	CMDName *m_mdname_rel;

	// list of columns
	gpos::owner<CDXLColDescrArray *> m_col_descr_array;

	// storage options
	gpos::owner<CDXLCtasStorageOptions *> m_dxl_ctas_storage_option;

	// distribution policy
	IMDRelation::Ereldistrpolicy m_rel_distr_policy;

	// list of distribution column positions
	gpos::owner<ULongPtrArray *> m_distr_column_pos_array;

	// list of distribution column opfamilies
	gpos::owner<IMdIdArray *> m_distr_opfamilies;

	// list of distribution column opclasses for populating dist policy of created table
	gpos::owner<IMdIdArray *> m_distr_opclasses;

	// is this a temporary table
	BOOL m_is_temp_table;

	// does table have oids
	BOOL m_has_oids;

	// storage type
	IMDRelation::Erelstoragetype m_rel_storage_type;

	// list of source column ids
	gpos::owner<ULongPtrArray *> m_src_colids_array;

	// list of vartypmod for target expressions
	// typemod records type-specific, e.g. the maximum length of a character column
	gpos::owner<IntPtrArray *> m_vartypemod_array;

public:
	CDXLLogicalCTAS(const CDXLLogicalCTAS &) = delete;

	// ctor
	CDXLLogicalCTAS(
		CMemoryPool *mp, gpos::owner<IMDId *> mdid, CMDName *mdname_schema,
		CMDName *mdname_rel,
		gpos::owner<CDXLColDescrArray *> dxl_col_descr_array,
		gpos::owner<CDXLCtasStorageOptions *> dxl_ctas_storage_option,
		IMDRelation::Ereldistrpolicy rel_distr_policy,
		gpos::owner<ULongPtrArray *> distr_column_pos_array,
		gpos::owner<IMdIdArray *> distr_opfamilies,
		gpos::owner<IMdIdArray *> distr_opclasses, BOOL fTemporary,
		BOOL fHasOids, IMDRelation::Erelstoragetype rel_storage_type,
		gpos::owner<ULongPtrArray *> src_colids_array,
		gpos::owner<IntPtrArray *> vartypemod_array);

	// dtor
	~CDXLLogicalCTAS() override;

	// operator type
	Edxlopid GetDXLOperator() const override;

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// mdid of table to create
	gpos::pointer<IMDId *>
	MDId() const
	{
		return m_mdid;
	}

	// schema name
	CMDName *
	GetMdNameSchema() const
	{
		return m_mdname_schema;
	}

	// table name
	CMDName *
	MdName() const
	{
		return m_mdname_rel;
	}

	// column descriptors
	gpos::pointer<CDXLColDescrArray *>
	GetDXLColumnDescrArray() const
	{
		return m_col_descr_array;
	}

	// storage type
	IMDRelation::Erelstoragetype
	RetrieveRelStorageType() const
	{
		return m_rel_storage_type;
	}

	// distribution policy
	IMDRelation::Ereldistrpolicy
	Ereldistrpolicy() const
	{
		return m_rel_distr_policy;
	}

	// distribution column positions
	gpos::pointer<ULongPtrArray *>
	GetDistrColPosArray() const
	{
		return m_distr_column_pos_array;
	}

	// distribution column opfamilies
	gpos::pointer<IMdIdArray *>
	GetDistrOpfamilies() const
	{
		return m_distr_opfamilies;
	}

	// distribution column opclasses
	gpos::pointer<IMdIdArray *>
	GetDistrOpclasses() const
	{
		return m_distr_opclasses;
	}

	// source column ids
	gpos::pointer<ULongPtrArray *>
	GetSrcColidsArray() const
	{
		return m_src_colids_array;
	}

	// list of vartypmod for target expressions
	gpos::pointer<IntPtrArray *>
	GetVarTypeModArray() const
	{
		return m_vartypemod_array;
	}

	// is it a temporary table
	BOOL
	IsTemporary() const
	{
		return m_is_temp_table;
	}

	// does the table have oids
	BOOL
	HasOids() const
	{
		return m_has_oids;
	}

	// CTAS storage options
	gpos::pointer<CDXLCtasStorageOptions *>
	GetDxlCtasStorageOption() const
	{
		return m_dxl_ctas_storage_option;
	}

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(gpos::pointer<const CDXLNode *> dxlnode,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG

	// check if given column is defined by operator
	BOOL IsColDefined(ULONG colid) const override;

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						gpos::pointer<const CDXLNode *> dxlnode) const override;

	// conversion function
	static gpos::cast_func<CDXLLogicalCTAS *>
	Cast(CDXLOperator *dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopLogicalCTAS == dxl_op->GetDXLOperator());

		return dynamic_cast<CDXLLogicalCTAS *>(dxl_op);
	}
};
}  // namespace gpdxl

#endif	// !GPDXL_CDXLLogicalCTAS_H

// EOF
