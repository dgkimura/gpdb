//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CMDRelationCtasGPDB.h
//
//	@doc:
//		Class representing MD CTAS relations
//---------------------------------------------------------------------------

#ifndef GPMD_CMDRelationCTASGPDB_H
#define GPMD_CMDRelationCTASGPDB_H

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDColumn.h"
#include "naucrates/md/CMDName.h"
#include "naucrates/md/IMDColumn.h"
#include "naucrates/md/IMDRelationCtas.h"
#include "naucrates/statistics/IStatistics.h"

namespace gpdxl
{
// fwd decl
class CXMLSerializer;
class CDXLCtasStorageOptions;
}  // namespace gpdxl

namespace gpmd
{
using namespace gpos;
using namespace gpdxl;


//---------------------------------------------------------------------------
//	@class:
//		CMDRelationCtasGPDB
//
//	@doc:
//		Class representing MD CTAS relations
//
//---------------------------------------------------------------------------
class CMDRelationCtasGPDB : public IMDRelationCtas
{
private:
	// memory pool
	CMemoryPool *m_mp;

	// DXL for object
	const CWStringDynamic *m_dxl_str;

	// relation mdid
	gpos::Ref<IMDId> m_mdid;

	// schema name
	CMDName *m_mdname_schema;

	// table name
	CMDName *m_mdname;

	// is this a temporary relation
	BOOL m_is_temp_table;

	// storage type
	Erelstoragetype m_rel_storage_type;

	// distribution policy
	Ereldistrpolicy m_rel_distr_policy;

	// columns
	gpos::Ref<CMDColumnArray> m_md_col_array;

	// indices of distribution columns
	gpos::Ref<ULongPtrArray> m_distr_col_array;

	// distribution opfamilies
	gpos::Ref<IMdIdArray> m_distr_opfamilies;

	// distribution opclasses
	gpos::Ref<IMdIdArray> m_distr_opclasses;

	// array of key sets
	gpos::Ref<ULongPtr2dArray> m_keyset_array;

	// number of system columns
	ULONG m_system_columns;

	// mapping of attribute number in the system catalog to the positions of
	// the non dropped column in the metadata object
	gpos::Ref<IntToUlongMap> m_attrno_nondrop_col_pos_map;

	// the original positions of all the non-dropped columns
	gpos::Ref<ULongPtrArray> m_nondrop_col_pos_array;

	// storage options
	gpos::Ref<CDXLCtasStorageOptions> m_dxl_ctas_storage_option;

	// vartypemod list
	gpos::Ref<IntPtrArray> m_vartypemod_array;

	// array of column widths
	gpos::Ref<CDoubleArray> m_col_width_array;

public:
	CMDRelationCtasGPDB(const CMDRelationCtasGPDB &) = delete;

	// ctor
	CMDRelationCtasGPDB(
		CMemoryPool *mp, gpos::Ref<IMDId> mdid, CMDName *mdname_schema,
		CMDName *mdname, BOOL fTemporary, Erelstoragetype rel_storage_type,
		Ereldistrpolicy rel_distr_policy, gpos::Ref<CMDColumnArray> mdcol_array,
		gpos::Ref<ULongPtrArray> distr_col_array,
		gpos::Ref<IMdIdArray> distr_opfamilies,
		gpos::Ref<IMdIdArray> distr_opclasses,
		gpos::Ref<ULongPtr2dArray> keyset_array,
		gpos::Ref<CDXLCtasStorageOptions> dxl_ctas_storage_options,
		gpos::Ref<IntPtrArray> vartypemod_array);

	// dtor
	~CMDRelationCtasGPDB() override;

	// accessors
	const CWStringDynamic *
	GetStrRepr() const override
	{
		return m_dxl_str;
	}

	// the metadata id
	gpos::Ref<IMDId> MDId() const override;

	// schema name
	CMDName *GetMdNameSchema() const override;

	// relation name
	CMDName Mdname() const override;

	// distribution policy (none, hash, random)
	Ereldistrpolicy GetRelDistribution() const override;

	// is this a temp relation
	BOOL
	IsTemporary() const override
	{
		return m_is_temp_table;
	}

	// storage type
	Erelstoragetype
	RetrieveRelStorageType() const override
	{
		return m_rel_storage_type;
	}

	// CTAS storage options
	gpos::Ref<CDXLCtasStorageOptions>
	GetDxlCtasStorageOption() const override
	{
		return m_dxl_ctas_storage_option.get();
	}

	// number of columns
	ULONG ColumnCount() const override;

	// width of a column with regards to the position
	DOUBLE ColWidth(ULONG pos) const override;

	// does relation have dropped columns
	BOOL
	HasDroppedColumns() const override
	{
		return false;
	}

	// number of non-dropped columns
	ULONG
	NonDroppedColsCount() const override
	{
		return ColumnCount();
	}

	// return the original positions of all the non-dropped columns
	gpos::Ref<ULongPtrArray>
	NonDroppedColsArray() const override
	{
		return m_nondrop_col_pos_array.get();
	}

	// number of system columns
	ULONG SystemColumnsCount() const override;

	// retrieve the column at the given position
	const IMDColumn *GetMdCol(ULONG pos) const override;

	// number of distribution columns
	ULONG DistrColumnCount() const override;

	// retrieve the column at the given position in the distribution columns list for the relation
	const IMDColumn *GetDistrColAt(ULONG pos) const override;

	gpos::Ref<IMDId> GetDistrOpfamilyAt(ULONG pos) const override;

	// number of indices
	ULONG
	IndexCount() const override
	{
		return 0;
	}

	// number of triggers
	ULONG
	TriggerCount() const override
	{
		return 0;
	}

	// return the absolute position of the given attribute position excluding dropped columns
	ULONG
	NonDroppedColAt(ULONG pos) const override
	{
		return pos;
	}

	// return the position of a column in the metadata object given the attribute number in the system catalog
	ULONG GetPosFromAttno(INT attno) const override;

	virtual gpos::Ref<IMdIdArray>
	GetDistrOpClasses() const
	{
		return m_distr_opclasses.get();
	}

	// retrieve the id of the metadata cache index at the given position
	gpos::Ref<IMDId> IndexMDidAt(ULONG	// pos
	) const override
	{
		GPOS_ASSERT("CTAS tables have no indexes");
		return nullptr;
	}

	// retrieve the id of the metadata cache trigger at the given position
	gpos::Ref<IMDId> TriggerMDidAt(ULONG  // pos
	) const override
	{
		GPOS_ASSERT("CTAS tables have no triggers");
		return nullptr;
	}

	// serialize metadata relation in DXL format given a serializer object
	void Serialize(gpdxl::CXMLSerializer *) const override;

	// number of check constraints
	ULONG
	CheckConstraintCount() const override
	{
		return 0;
	}

	// retrieve the id of the check constraint cache at the given position
	gpos::Ref<IMDId> CheckConstraintMDidAt(ULONG  // pos
	) const override
	{
		GPOS_ASSERT("CTAS tables have no constraints");
		return nullptr;
	}

	// list of vartypmod for target expressions
	IntPtrArray *
	GetVarTypeModArray() const
	{
		return m_vartypemod_array.get();
	}

#ifdef GPOS_DEBUG
	// debug print of the metadata relation
	void DebugPrint(IOstream &os) const override;
#endif
};
}  // namespace gpmd

#endif	// !GPMD_CMDRelationCTASGPDB_H

// EOF
