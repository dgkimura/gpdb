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
	gpos::Ref<IMDId> m_mdid;

	// schema name
	CMDName *m_mdname_schema;

	// table name
	CMDName *m_mdname_rel;

	// list of columns
	gpos::Ref<CDXLColDescrArray> m_col_descr_array;

	// storage options
	gpos::Ref<CDXLCtasStorageOptions> m_dxl_ctas_storage_option;

	// distribution policy
	IMDRelation::Ereldistrpolicy m_rel_distr_policy;

	// list of distribution column positions
	gpos::Ref<ULongPtrArray> m_distr_column_pos_array;

	// list of distribution column opfamilies
	gpos::Ref<IMdIdArray> m_distr_opfamilies;

	// list of distribution column opclasses for populating dist policy of created table
	gpos::Ref<IMdIdArray> m_distr_opclasses;

	// is this a temporary table
	BOOL m_is_temp_table;

	// does table have oids
	BOOL m_has_oids;

	// storage type
	IMDRelation::Erelstoragetype m_rel_storage_type;

	// list of source column ids
	gpos::Ref<ULongPtrArray> m_src_colids_array;

	// list of vartypmod for target expressions
	// typemod records type-specific, e.g. the maximum length of a character column
	gpos::Ref<IntPtrArray> m_vartypemod_array;

public:
	CDXLLogicalCTAS(const CDXLLogicalCTAS &) = delete;

	// ctor
	CDXLLogicalCTAS(CMemoryPool *mp, gpos::Ref<IMDId> mdid,
					CMDName *mdname_schema, CMDName *mdname_rel,
					gpos::Ref<CDXLColDescrArray> dxl_col_descr_array,
					gpos::Ref<CDXLCtasStorageOptions> dxl_ctas_storage_option,
					IMDRelation::Ereldistrpolicy rel_distr_policy,
					gpos::Ref<ULongPtrArray> distr_column_pos_array,
					gpos::Ref<IMdIdArray> distr_opfamilies,
					gpos::Ref<IMdIdArray> distr_opclasses, BOOL fTemporary,
					BOOL fHasOids,
					IMDRelation::Erelstoragetype rel_storage_type,
					gpos::Ref<ULongPtrArray> src_colids_array,
					gpos::Ref<IntPtrArray> vartypemod_array);

	// dtor
	~CDXLLogicalCTAS() override;

	// operator type
	Edxlopid GetDXLOperator() const override;

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// mdid of table to create
	IMDId *
	MDId() const
	{
		return m_mdid.get();
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
	CDXLColDescrArray *
	GetDXLColumnDescrArray() const
	{
		return m_col_descr_array.get();
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
	ULongPtrArray *
	GetDistrColPosArray() const
	{
		return m_distr_column_pos_array.get();
	}

	// distribution column opfamilies
	IMdIdArray *
	GetDistrOpfamilies() const
	{
		return m_distr_opfamilies.get();
	}

	// distribution column opclasses
	IMdIdArray *
	GetDistrOpclasses() const
	{
		return m_distr_opclasses.get();
	}

	// source column ids
	ULongPtrArray *
	GetSrcColidsArray() const
	{
		return m_src_colids_array.get();
	}

	// list of vartypmod for target expressions
	IntPtrArray *
	GetVarTypeModArray() const
	{
		return m_vartypemod_array.get();
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
	CDXLCtasStorageOptions *
	GetDxlCtasStorageOption() const
	{
		return m_dxl_ctas_storage_option.get();
	}

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(const CDXLNode *dxlnode,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG

	// check if given column is defined by operator
	BOOL IsColDefined(ULONG colid) const override;

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						const CDXLNode *dxlnode) const override;

	// conversion function
	static CDXLLogicalCTAS *
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
