//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CTranslatorUtils.h
//
//	@doc:
//		Class providing utility methods for translating GPDB's PlannedStmt/Query
//		into DXL Tree
//
//	@test:
//
//---------------------------------------------------------------------------

#ifndef GPDXL_CTranslatorUtils_H
#define GPDXL_CTranslatorUtils_H
#include "gpos/common/owner.h"
#define GPDXL_SYSTEM_COLUMNS 8

extern "C" {
#include "postgres.h"

#include "access/sdir.h"
#include "access/skey.h"
#include "nodes/parsenodes.h"
}

#include "gpos/base.h"
#include "gpos/common/CBitSet.h"

#include "gpopt/translate/CMappingVarColId.h"
#include "naucrates/dxl/CIdGenerator.h"
#include "naucrates/dxl/operators/CDXLIndexDescr.h"
#include "naucrates/dxl/operators/CDXLLogicalSetOp.h"
#include "naucrates/dxl/operators/CDXLLogicalTVF.h"
#include "naucrates/dxl/operators/CDXLPhysicalDML.h"
#include "naucrates/dxl/operators/CDXLPhysicalMotion.h"
#include "naucrates/dxl/operators/CDXLScalarAggref.h"
#include "naucrates/dxl/operators/CDXLScalarSubPlan.h"
#include "naucrates/md/CMDRelationGPDB.h"
#include "naucrates/md/IMDType.h"
#include "naucrates/statistics/IStatistics.h"


// fwd declarations
namespace gpopt
{
class CMDAccessor;

// dynamic array of bitsets
typedef CDynamicPtrArray<CBitSet, CleanupRelease> CBitSetArray;
}  // namespace gpopt

namespace gpdxl
{
class CDXLTranslateContext;
}

namespace gpdxl
{
using namespace gpopt;

enum DistributionHashOpsKind
{
	DistrHashOpsNotDeterminedYet,
	DistrUseDefaultHashOps,
	DistrUseLegacyHashOps
};

//---------------------------------------------------------------------------
//	@class:
//		CTranslatorUtils
//
//	@doc:
//		Class providing methods for translating GPDB's PlannedStmt/Query
//		into DXL Tree
//
//---------------------------------------------------------------------------
class CTranslatorUtils
{
private:
	// Construct a set of column attnos corresponding to a single grouping set
	// from either a plain GROUP BY or one set in a list of grouping sets
	static gpos::owner<CBitSet *> CreateAttnoSetForGroupingSet(
		CMemoryPool *mp, List *group_elems, ULONG num_cols,
		gpos::pointer<UlongToUlongMap *> group_col_pos,
		gpos::pointer<CBitSet *> group_cols, bool use_group_clause);

	// check if the given mdid array contains any of the polymorphic
	// types (ANYELEMENT, ANYARRAY)
	static BOOL ContainsPolymorphicTypes(
		gpos::pointer<IMdIdArray *> mdid_array);

	// resolve polymorphic types in the given array of type ids, replacing
	// them with the actual types obtained from the query
	static gpos::owner<IMdIdArray *> ResolvePolymorphicTypes(
		CMemoryPool *mp, gpos::pointer<IMdIdArray *> return_arg_mdids,
		List *input_arg_types, FuncExpr *funcexpr);

	// update grouping col position mappings
	static void UpdateGrpColMapping(
		CMemoryPool *mp,
		gpos::pointer<UlongToUlongMap *> grouping_col_to_pos_map,
		gpos::pointer<CBitSet *> group_cols, ULONG sort_group_ref);

	// create a set of grouping sets for a rollup
	static gpos::owner<CBitSetArray *> CreateGroupingSetsForRollup(
		CMemoryPool *mp, const GroupingSet *grouping_set, ULONG num_cols,
		gpos::pointer<CBitSet *> group_cols,
		gpos::pointer<UlongToUlongMap *> group_col_pos);

	// create a set of grouping sets for a grouping sets subclause
	static gpos::owner<CBitSetArray *> CreateGroupingSetsForSets(
		CMemoryPool *mp, const GroupingSet *grouping_set_node, ULONG num_cols,
		gpos::pointer<CBitSet *> group_cols,
		gpos::pointer<UlongToUlongMap *> group_col_pos);

public:
	struct SCmptypeStrategy
	{
		IMDType::ECmpType comptype;
		StrategyNumber strategy_no;
	};

	// get the GPDB scan direction from its corresponding DXL representation
	static ScanDirection GetScanDirection(
		EdxlIndexScanDirection idx_scan_direction);

	// get the oid of comparison operator
	static OID OidCmpOperator(Expr *expr);

	// get the opfamily for index key
	static OID GetOpFamilyForIndexQual(INT attno, OID oid_index);

	// return the type for the system column with the given number
	static gpos::owner<CMDIdGPDB *> GetSystemColType(CMemoryPool *mp,
													 AttrNumber attno);

	// find the n-th column descriptor in the table descriptor
	static gpos::pointer<const CDXLColDescr *> GetColumnDescrAt(
		gpos::pointer<const CDXLTableDescr *> table_descr, ULONG pos);

	// return the name for the system column with given number
	static const CWStringConst *GetSystemColName(AttrNumber attno);

	// returns the length for the system column with given attno number
	static const ULONG GetSystemColLength(AttrNumber attno);

	// translate the join type from its GPDB representation into the DXL one
	static EdxlJoinType ConvertToDXLJoinType(JoinType jt);

	// translate the index scan direction from its GPDB representation into the DXL one
	static EdxlIndexScanDirection ConvertToDXLIndexScanDirection(
		ScanDirection sd);

	// create a DXL index descriptor from an index MD id
	static gpos::owner<CDXLIndexDescr *> GetIndexDescr(
		CMemoryPool *mp, CMDAccessor *md_accessor, gpos::owner<IMDId *> mdid);

	// translate a RangeTableEntry into a CDXLTableDescr
	static gpos::owner<CDXLTableDescr *> GetTableDescr(
		CMemoryPool *mp, CMDAccessor *md_accessor, CIdGenerator *id_generator,
		const RangeTblEntry *rte, BOOL *is_distributed_table = nullptr);

	// translate a RangeTableEntry into a CDXLLogicalTVF
	static gpos::owner<CDXLLogicalTVF *> ConvertToCDXLLogicalTVF(
		CMemoryPool *mp, CMDAccessor *md_accessor, CIdGenerator *id_generator,
		const RangeTblEntry *rte);

	// get column descriptors from a record type
	static gpos::owner<CDXLColDescrArray *> GetColumnDescriptorsFromRecord(
		CMemoryPool *mp, CIdGenerator *id_generator, List *col_names,
		List *col_types, List *col_type_modifiers);

	// get column descriptors from a record type
	static gpos::owner<CDXLColDescrArray *> GetColumnDescriptorsFromRecord(
		CMemoryPool *mp, CIdGenerator *id_generator, List *col_names,
		gpos::pointer<IMdIdArray *> out_arg_types);

	// get column descriptor from a base type
	static gpos::owner<CDXLColDescrArray *> GetColumnDescriptorsFromBase(
		CMemoryPool *mp, CIdGenerator *id_generator,
		gpos::pointer<IMDId *> mdid_return_type, INT type_modifier,
		CMDName *md_name);

	// get column descriptors from a composite type
	static gpos::owner<CDXLColDescrArray *> GetColumnDescriptorsFromComposite(
		CMemoryPool *mp, CMDAccessor *md_accessor, CIdGenerator *id_generator,
		gpos::pointer<const IMDType *> md_type);

	// expand a composite type into an array of IMDColumns
	static gpos::owner<CMDColumnArray *> ExpandCompositeType(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::pointer<const IMDType *> md_type);

	// return the dxl representation of the set operation
	static EdxlSetOpType GetSetOpType(SetOperation setop, BOOL is_all);

	// construct a dynamic array of sets of column attnos corresponding
	// to the group by clause
	static gpos::owner<CBitSetArray *> GetColumnAttnosForGroupBy(
		CMemoryPool *mp, List *group_clause, List *grouping_set_list,
		ULONG num_cols, gpos::pointer<UlongToUlongMap *> group_col_pos,
		gpos::pointer<CBitSet *> group_cold);

	// return a copy of the query with constant of unknown type being coerced
	// to the common data type of the output target list
	static Query *FixUnknownTypeConstant(Query *query, List *target_list);

	// return the type of the nth non-resjunked target list entry
	static OID GetTargetListReturnTypeOid(List *target_list, ULONG col_pos);

	// construct an array of DXL column identifiers for a target list
	static gpos::owner<ULongPtrArray *> GenerateColIds(
		CMemoryPool *mp, List *target_list,
		gpos::pointer<IMdIdArray *> input_mdids,
		gpos::pointer<ULongPtrArray *> input_nums, const BOOL *is_outer_ref,
		CIdGenerator *colid_generator);

	// construct an array of DXL column descriptors for a target list
	// using the column ids in the given array
	static gpos::owner<CDXLColDescrArray *> GetDXLColumnDescrArray(
		CMemoryPool *mp, List *target_list,
		gpos::pointer<ULongPtrArray *> colids, BOOL keep_res_junked);

	// return the positions of the target list entries included in the output
	static gpos::owner<ULongPtrArray *> GetPosInTargetList(
		CMemoryPool *mp, List *target_list, BOOL keep_res_junked);

	// construct a column descriptor from the given target entry, column identifier and position in the output
	static gpos::owner<CDXLColDescr *> GetColumnDescrAt(
		CMemoryPool *mp, TargetEntry *target_entry, ULONG colid, ULONG pos);

	// create a dummy project element to rename the input column identifier
	static gpos::owner<CDXLNode *> CreateDummyProjectElem(
		CMemoryPool *mp, ULONG colid_input, ULONG colid_output,
		gpos::pointer<CDXLColDescr *> dxl_col_descr);

	// construct a list of colids corresponding to the given target list
	// using the given attno->colid map
	static gpos::owner<ULongPtrArray *> GetOutputColIdsArray(
		CMemoryPool *mp, List *target_list,
		gpos::pointer<IntToUlongMap *> attno_to_colid_map);

	// construct an array of column ids for the given group by set
	static gpos::owner<ULongPtrArray *> GetGroupingColidArray(
		CMemoryPool *mp, gpos::pointer<CBitSet *> group_by_cols,
		gpos::pointer<IntToUlongMap *> sort_group_cols_to_colid_map);

	// return the Colid of column with given index
	static ULONG GetColId(INT index,
						  gpos::pointer<IntToUlongMap *> index_to_colid_map);

	// return the corresponding ColId for the given varno, varattno and querylevel
	static ULONG GetColId(ULONG query_level, INT varno, INT var_attno,
						  gpos::pointer<IMDId *> mdid,
						  CMappingVarColId *var_colid_mapping);

	// check to see if the target list entry is a sorting column
	static BOOL IsSortingColumn(const TargetEntry *target_entry,
								List *sort_clause_list);
	// check to see if the target list entry is used in the window reference
	static BOOL IsReferencedInWindowSpec(const TargetEntry *target_entry,
										 List *window_clause_list);

	// check if the project list contains AggRef with ORDER BY
	static BOOL HasOrderedAggRefInProjList(
		gpos::pointer<CDXLNode *> proj_list_dxlnode);

	// extract a matching target entry that is a window spec
	static TargetEntry *GetWindowSpecTargetEntry(Node *node,
												 List *window_clause_list,
												 List *target_list);

	// create a scalar const value expression for the given int8 value
	static gpos::owner<CDXLNode *> CreateDXLProjElemFromInt8Const(
		CMemoryPool *mp, CMDAccessor *md_accessor, INT val);

	// check to see if the target list entry is a grouping column
	static BOOL IsGroupingColumn(const TargetEntry *target_entry,
								 List *group_clause_list);

	// check to see if the target list entry is a grouping column
	static BOOL IsGroupingColumn(const TargetEntry *target_entry,
								 const SortGroupClause *sort_group_clause);

	// check if the expression has a matching target entry that is a grouping column
	static BOOL IsGroupingColumn(Node *node, List *group_clause_list,
								 List *target_list);

	// extract a matching target entry that is a grouping column
	static TargetEntry *GetGroupingColumnTargetEntry(Node *node,
													 List *group_clause_list,
													 List *target_list);

	// convert a list of column ids to a list of attribute numbers using
	// the provided context with mappings
	static List *ConvertColidToAttnos(gpos::pointer<ULongPtrArray *> pdrgpul,
									  CDXLTranslateContext *dxl_translate_ctxt);

	// parse string value into a Long Integer
	static LINT GetLongFromStr(const CWStringBase *wcstr);

	// parse string value into an Integer
	static INT GetIntFromStr(const CWStringBase *wcstr);

	// check whether the given project list has a project element of the given
	// operator type
	static BOOL HasProjElem(gpos::pointer<CDXLNode *> project_list_dxlnode,
							Edxlopid dxl_op_id);

	// create a multi-byte character string from a wide character string
	static CHAR *CreateMultiByteCharStringFromWCString(const WCHAR *wcstr);

	static gpos::owner<UlongToUlongMap *> MakeNewToOldColMapping(
		CMemoryPool *mp, gpos::pointer<ULongPtrArray *> old_colids,
		gpos::pointer<ULongPtrArray *> new_colids);

	// check if the given tree contains a subquery
	static BOOL HasSubquery(Node *node);

	// check if the given function is a SIRV (single row volatile) that reads
	// or modifies SQL data
	static BOOL IsSirvFunc(CMemoryPool *mp, CMDAccessor *md_accessor,
						   OID func_oid);

	// is this a motion sensitive to duplicates
	static BOOL IsDuplicateSensitiveMotion(
		gpos::pointer<gpdxl::CDXLPhysicalMotion *> dxl_motion);

	// construct a project element with a const NULL expression
	static gpos::owner<CDXLNode *> CreateDXLProjElemConstNULL(
		CMemoryPool *mp, CMDAccessor *md_accessor, gpos::pointer<IMDId *> mdid,
		ULONG colid, const WCHAR *col_name);

	// construct a project element with a const NULL expression
	static gpos::owner<CDXLNode *> CreateDXLProjElemConstNULL(
		CMemoryPool *mp, CMDAccessor *md_accessor, gpos::pointer<IMDId *> mdid,
		ULONG colid, CHAR *alias_name);

	// create a DXL project element node with a Const NULL of type provided
	// by the column descriptor
	static gpos::owner<CDXLNode *> CreateDXLProjElemConstNULL(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		CIdGenerator *colid_generator, gpos::pointer<const IMDColumn *> col);

	// check required permissions for the range table
	static void CheckRTEPermissions(List *range_table_list);

	// check if given column ids are outer references in the tree rooted by given node
	static void MarkOuterRefs(ULONG *colid, BOOL *is_outer_ref,
							  ULONG num_columns,
							  gpos::pointer<CDXLNode *> node);

	// map DXL Subplan type to GPDB SubLinkType
	static SubLinkType MapDXLSubplanToSublinkType(
		gpdxl::EdxlSubPlanType dxl_subplan_type);

	// map GPDB SubLinkType to DXL Subplan type
	static EdxlSubPlanType MapSublinkTypeToDXLSubplan(SubLinkType slink);

	// check whether there are triggers for the given operation on
	// the given relation
	static BOOL RelHasTriggers(CMemoryPool *mp, CMDAccessor *md_accessor,
							   gpos::pointer<const IMDRelation *> mdrel,
							   const gpdxl::EdxlDmlType dml_type_dxl);

	// check whether the given trigger is applicable to the given DML operation
	static BOOL IsApplicableTrigger(CMDAccessor *md_accessor,
									gpos::pointer<IMDId *> trigger_mdid,
									const gpdxl::EdxlDmlType dml_type_dxl);

	// check whether there are NOT NULL or CHECK constraints for the given relation
	static BOOL RelHasConstraints(gpos::pointer<const IMDRelation *> rel);

	// translate the list of error messages from an assert constraint list
	static List *GetAssertErrorMsgs(
		gpos::pointer<CDXLNode *> assert_constraint_list);

	// return the count of non-system columns in the relation
	static ULONG GetNumNonSystemColumns(
		gpos::pointer<const IMDRelation *> mdrel);

	// return agg kind as an EdxlAggrefKind
	static EdxlAggrefKind GetAggKind(CHAR aggkind);

	// return agg kind as a CHAR
	static CHAR GetAggKind(EdxlAggrefKind aggkind);

	// check if const func returns composite type
	static BOOL IsCompositeConst(CMemoryPool *mp, CMDAccessor *md_accessor,
								 const RangeTblFunction *rtfunc);
};
}  // namespace gpdxl

#endif	// !GPDXL_CTranslatorUtils_H

// EOF
