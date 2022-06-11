//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CDXLPhysicalCTAS.h
//
//	@doc:
//		Class for representing DXL physical CTAS operators
//---------------------------------------------------------------------------
#ifndef GPDXL_CDXLPhysicalCTAS_H
#define GPDXL_CDXLPhysicalCTAS_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLColDescr.h"
#include "naucrates/dxl/operators/CDXLPhysical.h"
#include "naucrates/md/IMDRelation.h"

namespace gpdxl
{
// fwd decl
class CDXLCtasStorageOptions;

using namespace gpmd;

//---------------------------------------------------------------------------
//	@class:
//		CDXLPhysicalCTAS
//
//	@doc:
//		Class for representing DXL physical CTAS operators
//
//---------------------------------------------------------------------------
class CDXLPhysicalCTAS : public CDXLPhysical
{
private:
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

	// list of distriution column opclasses
	gpos::Ref<IMdIdArray> m_distr_opclasses;

	// is this a temporary table
	BOOL m_is_temp_table;

	// does table have oids
	BOOL m_has_oids;

	// storage type
	IMDRelation::Erelstoragetype m_rel_storage_type;

	// list of source column ids
	gpos::Ref<ULongPtrArray> m_src_colids_array;

	// list of vartypmod
	gpos::Ref<IntPtrArray> m_vartypemod_array;

public:
	CDXLPhysicalCTAS(CDXLPhysicalCTAS &) = delete;

	// ctor
	CDXLPhysicalCTAS(CMemoryPool *mp, CMDName *mdname_schema,
					 CMDName *mdname_rel,
					 gpos::Ref<CDXLColDescrArray> dxl_col_descr_array,
					 gpos::Ref<CDXLCtasStorageOptions> dxl_ctas_storage_options,
					 IMDRelation::Ereldistrpolicy rel_distr_policy,
					 gpos::Ref<ULongPtrArray> distr_column_pos_array,
					 gpos::Ref<IMdIdArray> distr_opclasses, BOOL is_temporary,
					 BOOL has_oids,
					 IMDRelation::Erelstoragetype rel_storage_type,
					 gpos::Ref<ULongPtrArray> src_colids_array,
					 gpos::Ref<IntPtrArray> vartypemod_array);

	// dtor
	~CDXLPhysicalCTAS() override;

	// operator type
	Edxlopid GetDXLOperator() const override;

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// column descriptors
	CDXLColDescrArray *
	GetDXLColumnDescrArray() const
	{
		return m_col_descr_array.get();
	}

	// distribution type
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

	// table name
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

	// is temporary
	BOOL
	IsTemporary() const
	{
		return m_is_temp_table;
	}

	// CTAS storage options
	CDXLCtasStorageOptions *
	GetDxlCtasStorageOption() const
	{
		return m_dxl_ctas_storage_option.get();
	}

	IMdIdArray *
	GetDistrOpclasses() const
	{
		return m_distr_opclasses.get();
	}
	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						const CDXLNode *dxlnode) const override;

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(const CDXLNode *dxlnode,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG

	// conversion function
	static CDXLPhysicalCTAS *
	Cast(CDXLOperator *dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopPhysicalCTAS == dxl_op->GetDXLOperator());
		return dynamic_cast<CDXLPhysicalCTAS *>(dxl_op);
	}
};
}  // namespace gpdxl
#endif	// !GPDXL_CDXLPhysicalCTAS_H

// EOF
