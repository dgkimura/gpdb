//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CMDRelationExternalGPDB.h
//
//	@doc:
//		Class representing MD external relations
//---------------------------------------------------------------------------

#ifndef GPMD_CMDRelationExternalGPDB_H
#define GPMD_CMDRelationExternalGPDB_H

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDColumn.h"
#include "naucrates/md/CMDName.h"
#include "naucrates/md/IMDColumn.h"
#include "naucrates/md/IMDRelationExternal.h"
#include "naucrates/statistics/IStatistics.h"

namespace gpdxl
{
class CXMLSerializer;
}

namespace gpmd
{
using namespace gpos;
using namespace gpdxl;


//---------------------------------------------------------------------------
//	@class:
//		CMDRelationExternalGPDB
//
//	@doc:
//		Class representing MD external relations
//
//---------------------------------------------------------------------------
class CMDRelationExternalGPDB : public IMDRelationExternal
{
private:
	// memory pool
	CMemoryPool *m_mp;

	// DXL for object
	const CWStringDynamic *m_dxl_str;

	// relation mdid
	gpos::owner<IMDId *> m_mdid;

	// table name
	CMDName *m_mdname;

	// distribution policy
	Ereldistrpolicy m_rel_distr_policy;

	// columns
	gpos::owner<CMDColumnArray *> m_md_col_array;

	// number of dropped columns
	ULONG m_dropped_cols;

	// indices of distribution columns
	gpos::owner<ULongPtrArray *> m_distr_col_array;

	gpos::owner<IMdIdArray *> m_distr_opfamilies;

	// do we need to consider a hash distributed table as random distributed
	BOOL m_convert_hash_to_random;

	// array of key sets
	gpos::owner<ULongPtr2dArray *> m_keyset_array;

	// array of index infos
	gpos::owner<CMDIndexInfoArray *> m_mdindex_info_array;

	// array of trigger ids
	gpos::owner<IMdIdArray *> m_mdid_trigger_array;

	// array of check constraint mdids
	gpos::owner<IMdIdArray *> m_mdid_check_constraint_array;

	// reject limit
	INT m_reject_limit;

	// reject limit specified as number of rows (as opposed to a percentage)?
	BOOL m_is_rej_limit_in_rows;

	// format error table mdid
	gpos::owner<IMDId *> m_mdid_fmt_err_table;

	// number of system columns
	ULONG m_system_columns;

	// mapping of column position to positions excluding dropped columns
	gpos::owner<UlongToUlongMap *> m_colpos_nondrop_colpos_map;

	// mapping of attribute number in the system catalog to the positions of
	// the non dropped column in the metadata object
	gpos::owner<IntToUlongMap *> m_attrno_nondrop_col_pos_map;

	// the original positions of all the non-dropped columns
	gpos::owner<ULongPtrArray *> m_nondrop_col_pos_array;

	// array of column widths including dropped columns
	gpos::owner<CDoubleArray *> m_col_width_array;

	// format type for the relation
	const CWStringConst *GetRelFormatType() const;

	// private copy ctor
	CMDRelationExternalGPDB(const CMDRelationExternalGPDB &);

public:
	// ctor
	CMDRelationExternalGPDB(
		CMemoryPool *mp, gpos::owner<IMDId *> mdid, CMDName *mdname,
		Ereldistrpolicy rel_distr_policy,
		gpos::owner<CMDColumnArray *> mdcol_array,
		gpos::owner<ULongPtrArray *> distr_col_array,
		gpos::owner<IMdIdArray *> distr_opfamilies, BOOL convert_hash_to_random,
		gpos::owner<ULongPtr2dArray *> keyset_array,
		gpos::owner<CMDIndexInfoArray *> md_index_info_array,
		gpos::owner<IMdIdArray *> mdid_triggers_array,
		gpos::owner<IMdIdArray *> mdid_check_constraint_array, INT reject_limit,
		BOOL is_reject_limit_in_rows, gpos::owner<IMDId *> mdid_fmt_err_table);

	// dtor
	~CMDRelationExternalGPDB() override;

	// accessors
	const CWStringDynamic *
	GetStrRepr() const override
	{
		return m_dxl_str;
	}

	// the metadata id
	gpos::pointer<IMDId *> MDId() const override;

	// relation name
	CMDName Mdname() const override;

	// distribution policy (none, hash, random)
	Ereldistrpolicy GetRelDistribution() const override;

	// number of columns
	ULONG ColumnCount() const override;

	// width of a column with regards to the position
	DOUBLE ColWidth(ULONG pos) const override;

	// does relation have dropped columns
	BOOL HasDroppedColumns() const override;

	// number of non-dropped columns
	ULONG NonDroppedColsCount() const override;

	// return the original positions of all the non-dropped columns
	gpos::pointer<ULongPtrArray *> NonDroppedColsArray() const override;

	// number of system columns
	ULONG SystemColumnsCount() const override;

	// return true if a hash distributed table needs to be considered as random
	BOOL ConvertHashToRandom() const override;

	// reject limit
	INT RejectLimit() const override;

	// reject limit in rows?
	BOOL IsRejectLimitInRows() const override;

	// format error table mdid
	gpos::pointer<IMDId *> GetFormatErrTableMdid() const override;

	// retrieve the column at the given position
	gpos::pointer<const IMDColumn *> GetMdCol(ULONG pos) const override;

	// number of key sets
	ULONG KeySetCount() const override;

	// key set at given position
	gpos::pointer<const ULongPtrArray *> KeySetAt(ULONG pos) const override;

	// number of distribution columns
	ULONG DistrColumnCount() const override;

	// retrieve the column at the given position in the distribution columns list for the relation
	gpos::pointer<const IMDColumn *> GetDistrColAt(ULONG pos) const override;

	gpos::pointer<IMDId *> GetDistrOpfamilyAt(ULONG pos) const override;

	// number of indices
	ULONG IndexCount() const override;

	// number of triggers
	ULONG TriggerCount() const override;

	// return the absolute position of the given attribute position excluding dropped columns
	ULONG NonDroppedColAt(ULONG pos) const override;

	// return the position of a column in the metadata object given the attribute number in the system catalog
	ULONG GetPosFromAttno(INT attno) const override;

	// retrieve the id of the metadata cache index at the given position
	IMDId *IndexMDidAt(ULONG pos) const override;

	// retrieve the id of the metadata cache trigger at the given position
	gpos::pointer<IMDId *> TriggerMDidAt(ULONG pos) const override;

	// serialize metadata relation in DXL format given a serializer object
	void Serialize(gpdxl::CXMLSerializer *) const override;

	// number of check constraints
	ULONG CheckConstraintCount() const override;

	// retrieve the id of the check constraint cache at the given position
	gpos::pointer<IMDId *> CheckConstraintMDidAt(ULONG pos) const override;

#ifdef GPOS_DEBUG
	// debug print of the metadata relation
	void DebugPrint(IOstream &os) const override;
#endif
};
}  // namespace gpmd

#endif	// !GPMD_CMDRelationExternalGPDB_H

// EOF
