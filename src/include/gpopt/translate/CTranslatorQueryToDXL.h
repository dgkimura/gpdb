//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CTranslatorQueryToDXL.h
//
//	@doc:
//		Class providing methods for translating a GPDB Query object into a
//		DXL Tree
//
//	@test:
//
//
//---------------------------------------------------------------------------

#ifndef GPDXL_CTranslatorQueryToDXL_H
#define GPDXL_CTranslatorQueryToDXL_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/translate/CContextQueryToDXL.h"
#include "gpopt/translate/CMappingVarColId.h"
#include "gpopt/translate/CTranslatorScalarToDXL.h"
#include "gpopt/translate/CTranslatorUtils.h"
#include "naucrates/dxl/operators/CDXLCtasStorageOptions.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLWindowSpec.h"

// fwd declarations
namespace gpopt
{
class CMDAccessor;
}

struct Query;
struct RangeTblEntry;
struct Const;
struct List;
struct CommonTableExpr;

namespace gpdxl
{
using namespace gpos;
using namespace gpopt;

using UlongBoolHashMap =
	CHashMap<ULONG, BOOL, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
			 CleanupDelete<ULONG>, CleanupDelete<BOOL>>;

using IntUlongHashmapIter =
	CHashMapIter<INT, ULONG, gpos::HashValue<INT>, gpos::Equals<INT>,
				 CleanupDelete<INT>, CleanupDelete<ULONG>>;

//---------------------------------------------------------------------------
//	@class:
//		CTranslatorQueryToDXL
//
//	@doc:
//		Class providing methods for translating a GPDB Query object into a
//      DXL Tree.
//
//---------------------------------------------------------------------------
class CTranslatorQueryToDXL
{
	friend class CTranslatorScalarToDXL;

	// mapping RTEKind to WCHARs
	struct SRTENameElem
	{
		RTEKind m_rtekind;
		const WCHAR *m_rte_name;
	};

	// mapping CmdType to WCHARs
	struct SCmdNameElem
	{
		CmdType m_cmd_type;
		const WCHAR *m_cmd_name;
	};

	// pair of unsupported node tag and feature name
	struct SUnsupportedFeature
	{
		NodeTag node_tag;
		const WCHAR *m_feature_name;
	};

private:
	// context for the whole query
	CContextQueryToDXL *m_context;

	// memory pool
	CMemoryPool *m_mp;

	// source system id
	CSystemId m_sysid;

	// meta data accessor
	CMDAccessor *m_md_accessor;

	// scalar translator used to convert scalar operation into DXL.
	CTranslatorScalarToDXL *m_scalar_translator;

	// holds the var to col id information mapping
	CMappingVarColId *m_var_to_colid_map;

	// query being translated
	Query *m_query;

	// absolute level of query being translated
	ULONG m_query_level;

	// top query is a DML
	BOOL m_is_top_query_dml;

	// this is a CTAS query
	BOOL m_is_ctas_query;

	// hash map that maintains the list of CTEs defined at a particular query level
	gpos::owner<HMUlCTEListEntry *> m_query_level_to_cte_map;

	// query output columns
	gpos::owner<CDXLNodeArray *> m_dxl_query_output_cols;

	// list of CTE producers
	gpos::owner<CDXLNodeArray *> m_dxl_cte_producers;

	// CTE producer IDs defined at the current query level
	gpos::owner<UlongBoolHashMap *> m_cteid_at_current_query_level_map;

	//ctor
	// private constructor, called from the public factory function QueryToDXLInstance
	CTranslatorQueryToDXL(
		CContextQueryToDXL *context, CMDAccessor *md_accessor,
		const CMappingVarColId *var_colid_mapping, Query *query,
		ULONG query_level, BOOL is_top_query_dml,
		gpos::pointer<HMUlCTEListEntry *>
			query_level_to_cte_map	// hash map between query level -> list of CTEs defined at that level
	);

	// check for unsupported node types, throws an exception if an unsupported
	// node is found
	static void CheckUnsupportedNodeTypes(Query *query);

	// check for SIRV functions in the targetlist without a FROM clause and
	// throw an exception when found
	void CheckSirvFuncsWithoutFromClause(Query *query);

	// check for SIRV functions in the tree rooted at the given node
	BOOL HasSirvFunctions(Node *node) const;

	// translate FromExpr (in the GPDB query) into a CDXLLogicalJoin or CDXLLogicalGet
	CDXLNode *TranslateFromExprToDXL(FromExpr *from_expr);

	// translate set operations
	gpos::owner<CDXLNode *> TranslateSetOpToDXL(
		Node *setop_node, List *target_list,
		gpos::pointer<IntToUlongMap *> output_attno_to_colid_mapping);

	// create the set operation given its children, input and output columns
	gpos::owner<CDXLNode *> CreateDXLSetOpFromColumns(
		EdxlSetOpType setop_type, List *output_target_list,
		gpos::pointer<ULongPtrArray *> output_colids,
		gpos::owner<ULongPtr2dArray *> input_colids,
		gpos::owner<CDXLNodeArray *> children_dxlnodes,
		BOOL is_cast_across_input, BOOL keep_res_junked) const;

	// check if the set operation need to cast any of its input columns
	static BOOL SetOpNeedsCast(List *target_list,
							   gpos::pointer<IMdIdArray *> input_col_mdids);
	// translate a window operator
	gpos::owner<CDXLNode *> TranslateWindowToDXL(
		gpos::owner<CDXLNode *> child_dxlnode, List *target_list,
		List *window_clause, List *sort_clause,
		gpos::pointer<IntToUlongMap *> sort_col_attno_to_colid_mapping,
		gpos::pointer<IntToUlongMap *> output_attno_to_colid_mapping);

	// translate window spec
	gpos::owner<CDXLWindowSpecArray *> TranslateWindowSpecToDXL(
		List *window_clause,
		gpos::pointer<IntToUlongMap *> sort_col_attno_to_colid_mapping,
		gpos::pointer<CDXLNode *> project_list_dxlnode_node);

	// update window spec positions of LEAD/LAG functions
	void UpdateLeadLagWinSpecPos(
		gpos::pointer<CDXLNode *> project_list_dxlnode,
		gpos::pointer<CDXLWindowSpecArray *> window_specs_dxlnode) const;

	// manufacture window frame for lead/lag functions
	gpos::owner<CDXLWindowFrame *> CreateWindowFramForLeadLag(
		BOOL is_lead_func, CDXLNode *dxl_offset) const;

	// translate the child of a set operation
	gpos::owner<CDXLNode *> TranslateSetOpChild(
		Node *child_node, gpos::pointer<ULongPtrArray *> pdrgpul,
		gpos::pointer<IMdIdArray *> input_col_mdids, List *target_list);

	// return a dummy const table get
	gpos::owner<CDXLNode *> DXLDummyConstTableGet() const;

	// translate an Expr into CDXLNode
	gpos::owner<CDXLNode *> TranslateExprToDXL(Expr *expr);

	// translate the JoinExpr (inside FromExpr) into a CDXLLogicalJoin node
	gpos::owner<CDXLNode *> TranslateJoinExprInFromToDXL(JoinExpr *join_expr);

	// construct a group by node for a set of grouping columns
	gpos::owner<CDXLNode *> CreateSimpleGroupBy(
		List *target_list, List *group_clause, CBitSet *bitset, BOOL has_aggs,
		BOOL has_grouping_sets,	 // is this GB part of a GS query
		gpos::owner<CDXLNode *> child_dxlnode,
		gpos::pointer<IntToUlongMap *>
			phmiulSortGrpColsColId,	 // mapping sortgroupref -> ColId
		gpos::pointer<IntToUlongMap *>
			child_attno_colid_mapping,	// mapping attno->colid in child node
		gpos::pointer<IntToUlongMap *>
			output_attno_to_colid_mapping  // mapping attno -> ColId for output columns
	);

	// check if the argument of a DQA has already being used by another DQA
	static BOOL IsDuplicateDqaArg(List *dqa_list, Aggref *aggref);

	// translate a query with grouping sets
	gpos::owner<CDXLNode *> TranslateGroupingSets(
		FromExpr *from_expr, List *target_list, List *group_clause,
		List *grouping_set, BOOL has_aggs,
		gpos::pointer<IntToUlongMap *> phmiulSortGrpColsColId,
		gpos::pointer<IntToUlongMap *> output_attno_to_colid_mapping);

	// expand the grouping sets into a union all operator
	gpos::owner<CDXLNode *> CreateDXLUnionAllForGroupingSets(
		FromExpr *from_expr, List *target_list, List *group_clause,
		BOOL has_aggs, gpos::owner<CBitSetArray *> pdrgpbsGroupingSets,
		gpos::pointer<IntToUlongMap *> phmiulSortGrpColsColId,
		gpos::pointer<IntToUlongMap *> output_attno_to_colid_mapping,
		gpos::pointer<UlongToUlongMap *>
			grpcol_index_to_colid_mapping  // mapping pos->unique grouping columns for grouping func arguments
	);

	// construct a project node with NULL values for columns not included in the grouping set
	gpos::owner<CDXLNode *> CreateDXLProjectNullsForGroupingSets(
		List *target_list, gpos::owner<CDXLNode *> child_dxlnode,
		gpos::pointer<CBitSet *> bitset,
		gpos::pointer<IntToUlongMap *> sort_grouping_col_mapping,
		gpos::pointer<IntToUlongMap *> output_attno_to_colid_mapping,
		gpos::pointer<UlongToUlongMap *> grpcol_index_to_colid_mapping) const;

	// construct a project node with appropriate values for the grouping funcs in the given target list
	gpos::owner<CDXLNode *> CreateDXLProjectGroupingFuncs(
		List *target_list, gpos::owner<CDXLNode *> child_dxlnode,
		gpos::pointer<CBitSet *> bitset,
		gpos::pointer<IntToUlongMap *> output_attno_to_colid_mapping,
		gpos::pointer<UlongToUlongMap *> grpcol_index_to_colid_mapping,
		gpos::pointer<IntToUlongMap *> sort_grpref_to_colid_mapping) const;

	// add sorting and grouping column into the hash map
	void AddSortingGroupingColumn(
		TargetEntry *target_entry,
		gpos::pointer<IntToUlongMap *> phmiulSortGrpColsColId,
		ULONG colid) const;

	// translate the list of sorting columns
	gpos::owner<CDXLNodeArray *> TranslateSortColumsToDXL(
		List *sort_clause,
		gpos::pointer<IntToUlongMap *> col_attno_colid_mapping) const;

	// translate the list of partition-by column identifiers
	gpos::owner<ULongPtrArray *> TranslatePartColumns(
		List *sort_clause,
		gpos::pointer<IntToUlongMap *> col_attno_colid_mapping) const;

	gpos::owner<CDXLNode *> TranslateLimitToDXLGroupBy(
		List *plsortcl,			  // list of sort clauses
		Node *limit_count,		  // query node representing the limit count
		Node *limit_offset_node,  // query node representing the limit offset
		gpos::owner<CDXLNode *>
			dxlnode,  // the dxl node representing the subtree
		gpos::pointer<IntToUlongMap *>
			grpcols_to_colid_mapping  // the mapping between the position in the TargetList to the ColId
	);

	// throws an exception when RTE kind not yet supported
	[[noreturn]] static void UnsupportedRTEKind(RTEKind rtekind);

	// translate an entry of the from clause (this can either be FromExpr or JoinExpr)
	gpos::owner<CDXLNode *> TranslateFromClauseToDXL(Node *node);

	// translate the target list entries of the query into a logical project
	gpos::owner<CDXLNode *> TranslateTargetListToDXLProject(
		List *target_list, CDXLNode *child_dxlnode,
		gpos::pointer<IntToUlongMap *> group_col_to_colid_mapping,
		gpos::pointer<IntToUlongMap *> output_attno_to_colid_mapping,
		List *group_clause, BOOL is_aggref_expanded = false);

	// translate a target list entry or a join alias entry into a project element
	gpos::owner<CDXLNode *> TranslateExprToDXLProject(
		Expr *expr, const CHAR *alias_name, BOOL insist_new_colids = false);

	// translate a CTE into a DXL logical CTE operator
	gpos::owner<CDXLNode *> TranslateCTEToDXL(const RangeTblEntry *rte,
											  ULONG rti,
											  ULONG current_query_level);

	// translate a base table range table entry into a logical get
	gpos::owner<CDXLNode *> TranslateRTEToDXLLogicalGet(
		const RangeTblEntry *rte, ULONG rti,
		ULONG  //current_query_level
	);

	void NoteDistributionPolicyOpclasses(const RangeTblEntry *rte);

	// generate a DXL node from column values, where each column value is
	// either a datum or scalar expression represented as a project element.
	gpos::owner<CDXLNode *> TranslateColumnValuesToDXL(
		gpos::pointer<CDXLDatumArray *> dxl_datum_array,
		gpos::pointer<CDXLColDescrArray *> dxl_column_descriptors,
		gpos::pointer<CDXLNodeArray *> dxl_project_elements) const;

	// translate a value scan range table entry
	gpos::owner<CDXLNode *> TranslateValueScanRTEToDXL(
		const RangeTblEntry *rte, ULONG rti,
		ULONG  //current_query_level
	);

	// create a dxl node from a array of datums and project elements
	gpos::owner<CDXLNode *> TranslateTVFToDXL(const RangeTblEntry *rte,
											  ULONG rti,
											  ULONG	 //current_query_level
	);

	// translate a derived table into a DXL logical operator
	CDXLNode *TranslateDerivedTablesToDXL(const RangeTblEntry *rte, ULONG rti,
										  ULONG current_query_level);

	// create a DXL node representing the scalar constant "true"
	gpos::owner<CDXLNode *> CreateDXLConstValueTrue();

	// store mapping attno->colid
	void StoreAttnoColIdMapping(
		gpos::pointer<IntToUlongMap *> attno_to_colid_mapping, INT attno,
		ULONG colid) const;

	// construct an array of output columns
	gpos::owner<CDXLNodeArray *> CreateDXLOutputCols(
		List *target_list,
		gpos::pointer<IntToUlongMap *> attno_to_colid_mapping) const;

	// check for support command types, throws an exception when command type not yet supported
	static void CheckSupportedCmdType(Query *query);

	// check for supported range table entries, throws an exception when something is not yet supported
	static void CheckRangeTable(Query *query);

	// translate a select-project-join expression into DXL
	gpos::owner<CDXLNode *> TranslateSelectProjectJoinToDXL(
		List *target_list, FromExpr *from_expr,
		gpos::pointer<IntToUlongMap *> sort_group_attno_to_colid_mapping,
		gpos::pointer<IntToUlongMap *> output_attno_to_colid_mapping,
		List *group_clause);

	// translate a select-project-join expression into DXL and keep variables appearing
	// in aggregates and grouping columns in the output column map
	gpos::owner<CDXLNode *> TranslateSelectProjectJoinForGrpSetsToDXL(
		List *target_list, FromExpr *from_expr,
		gpos::pointer<IntToUlongMap *> sort_group_attno_to_colid_mapping,
		gpos::pointer<IntToUlongMap *> output_attno_to_colid_mapping,
		List *group_clause);

	// helper to check if OID is included in given array of OIDs
	static BOOL OIDFound(OID oid, const OID oids[], ULONG size);

	// check if given operator is lead() window function
	static BOOL IsLeadWindowFunc(gpos::pointer<CDXLOperator *> dxlop);

	// check if given operator is lag() window function
	static BOOL IsLagWindowFunc(gpos::pointer<CDXLOperator *> dxlop);

	// translate an insert query
	gpos::owner<CDXLNode *> TranslateInsertQueryToDXL();

	// translate a delete query
	gpos::owner<CDXLNode *> TranslateDeleteQueryToDXL();

	// translate an update query
	gpos::owner<CDXLNode *> TranslateUpdateQueryToDXL();

	// translate a CTAS query
	gpos::owner<CDXLNode *> TranslateCTASToDXL();
	// translate CTAS storage options
	gpos::owner<CDXLCtasStorageOptions::CDXLCtasOptionArray *>
	GetDXLCtasOptionArray(List *options,
						  IMDRelation::Erelstoragetype *storage_type);

	// extract storage option value from defelem
	CWStringDynamic *ExtractStorageOptionStr(DefElem *def_elem);

	// return resno -> colId mapping of columns to be updated
	gpos::owner<IntToUlongMap *> UpdatedColumnMapping();

	// obtain the ids of the ctid and segmentid columns for the target
	// table of a DML query
	void GetCtidAndSegmentId(ULONG *ctid, ULONG *segment_id);

	// translate a grouping func expression
	gpos::owner<CDXLNode *> TranslateGroupingFuncToDXL(
		const Expr *expr, gpos::pointer<CBitSet *> bitset,
		gpos::pointer<UlongToUlongMap *> grpcol_index_to_colid_mapping) const;

	// construct a list of CTE producers from the query's CTE list
	void ConstructCTEProducerList(List *cte_list, ULONG query_level);

	// construct a stack of CTE anchors for each CTE producer in the given array
	void ConstructCTEAnchors(gpos::pointer<CDXLNodeArray *> dxlnodes,
							 CDXLNode **dxl_cte_anchor_top,
							 gpos::owner<CDXLNode *> *dxl_cte_anchor_bottom);

	// generate an array of new column ids of the given size
	gpos::owner<ULongPtrArray *> GenerateColIds(CMemoryPool *mp,
												ULONG size) const;

	// extract an array of colids from the given column mapping
	gpos::owner<ULongPtrArray *> ExtractColIds(
		CMemoryPool *mp,
		gpos::pointer<IntToUlongMap *> attno_to_colid_mapping) const;

	// construct a new mapping based on the given one by replacing the colid in the "From" list
	// with the colid at the same position in the "To" list
	static gpos::owner<IntToUlongMap *> RemapColIds(
		CMemoryPool *mp, gpos::pointer<IntToUlongMap *> attno_to_colid_mapping,
		gpos::pointer<ULongPtrArray *> from_list_colids,
		gpos::pointer<ULongPtrArray *> to_list_colids);

	// true iff this query or one of its ancestors is a DML query
	BOOL IsDMLQuery();

public:
	CTranslatorQueryToDXL(const CTranslatorQueryToDXL &) = delete;

	// dtor
	~CTranslatorQueryToDXL();

	// query object
	const Query *
	Pquery() const
	{
		return m_query;
	}

	// does query have distributed tables
	BOOL
	HasDistributedTables() const
	{
		return m_context->m_has_distributed_tables;
	}

	// does query have distributed tables
	DistributionHashOpsKind
	GetDistributionHashOpsKind() const
	{
		return m_context->m_distribution_hashops;
	}

	// main translation routine for Query -> DXL tree
	CDXLNode *TranslateSelectQueryToDXL();

	// main driver
	gpos::owner<CDXLNode *> TranslateQueryToDXL();

	// return the list of output columns
	gpos::pointer<CDXLNodeArray *> GetQueryOutputCols() const;

	// return the list of CTEs
	gpos::pointer<CDXLNodeArray *> GetCTEs() const;

	// factory function
	static CTranslatorQueryToDXL *QueryToDXLInstance(CMemoryPool *mp,
													 CMDAccessor *md_accessor,
													 Query *query);
};
}  // namespace gpdxl
#endif	// GPDXL_CTranslatorQueryToDXL_H

//EOF
