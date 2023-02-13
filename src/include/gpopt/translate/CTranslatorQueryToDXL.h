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
	gpos::Ref<HMUlCTEListEntry> m_query_level_to_cte_map;

	// query output columns
	gpos::Ref<CDXLNodeArray> m_dxl_query_output_cols;

	// list of CTE producers
	gpos::Ref<CDXLNodeArray> m_dxl_cte_producers;

	// CTE producer IDs defined at the current query level
	gpos::Ref<UlongBoolHashMap> m_cteid_at_current_query_level_map;

	//ctor
	// private constructor, called from the public factory function QueryToDXLInstance
	CTranslatorQueryToDXL(
		CContextQueryToDXL *context, CMDAccessor *md_accessor,
		const CMappingVarColId *var_colid_mapping, Query *query,
		ULONG query_level, BOOL is_top_query_dml,
		HMUlCTEListEntry *
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
	gpos::Ref<CDXLNode> TranslateFromExprToDXL(FromExpr *from_expr);

	// translate set operations
	gpos::Ref<CDXLNode> TranslateSetOpToDXL(
		Node *setop_node, List *target_list,
		IntToUlongMap *output_attno_to_colid_mapping);

	// create the set operation given its children, input and output columns
	gpos::Ref<CDXLNode> CreateDXLSetOpFromColumns(
		EdxlSetOpType setop_type, List *output_target_list,
		ULongPtrArray *output_colids, gpos::Ref<ULongPtr2dArray> input_colids,
		gpos::Ref<CDXLNodeArray> children_dxlnodes, BOOL is_cast_across_input,
		BOOL keep_res_junked) const;

	// check if the set operation need to cast any of its input columns
	static BOOL SetOpNeedsCast(List *target_list,
							   gpos::Ref<IMdIdArray> input_col_mdids);
	// translate a window operator
	gpos::Ref<CDXLNode> TranslateWindowToDXL(
		gpos::Ref<CDXLNode> child_dxlnode, List *target_list,
		List *window_clause, List *sort_clause,
		IntToUlongMap *sort_col_attno_to_colid_mapping,
		IntToUlongMap *output_attno_to_colid_mapping);

	// translate window spec
	gpos::Ref<CDXLWindowSpecArray> TranslateWindowSpecToDXL(
		List *window_clause, IntToUlongMap *sort_col_attno_to_colid_mapping,
		gpos::Ref<CDXLNode> project_list_dxlnode_node);

	// update window spec positions of LEAD/LAG functions
	void UpdateLeadLagWinSpecPos(
		gpos::Ref<CDXLNode> project_list_dxlnode,
		CDXLWindowSpecArray *window_specs_dxlnode) const;

	// manufacture window frame for lead/lag functions
	gpos::Ref<CDXLWindowFrame> CreateWindowFramForLeadLag(
		BOOL is_lead_func, gpos::Ref<CDXLNode> dxl_offset) const;

	// translate the child of a set operation
	gpos::Ref<CDXLNode> TranslateSetOpChild(
		Node *child_node, ULongPtrArray *pdrgpul,
		gpos::Ref<IMdIdArray> input_col_mdids, List *target_list);

	// return a dummy const table get
	gpos::Ref<CDXLNode> DXLDummyConstTableGet() const;

	// translate an Expr into CDXLNode
	gpos::Ref<CDXLNode> TranslateExprToDXL(Expr *expr);

	// translate the JoinExpr (inside FromExpr) into a CDXLLogicalJoin node
	gpos::Ref<CDXLNode> TranslateJoinExprInFromToDXL(JoinExpr *join_expr);

	// construct a group by node for a set of grouping columns
	gpos::Ref<CDXLNode> CreateSimpleGroupBy(
		List *target_list, List *group_clause, gpos::Ref<CBitSet> bitset,
		BOOL has_aggs,
		BOOL has_grouping_sets,	 // is this GB part of a GS query
		gpos::Ref<CDXLNode> child_dxlnode,
		IntToUlongMap *phmiulSortGrpColsColId,	// mapping sortgroupref -> ColId
		IntToUlongMap
			*child_attno_colid_mapping,	 // mapping attno->colid in child node
		IntToUlongMap *
			output_attno_to_colid_mapping  // mapping attno -> ColId for output columns
	);

	// check if the argument of a DQA has already being used by another DQA
	static BOOL IsDuplicateDqaArg(List *dqa_list, Aggref *aggref);

	// translate a query with grouping sets
	gpos::Ref<CDXLNode> TranslateGroupingSets(
		FromExpr *from_expr, List *target_list, List *group_clause,
		List *grouping_set, BOOL has_aggs,
		IntToUlongMap *phmiulSortGrpColsColId,
		IntToUlongMap *output_attno_to_colid_mapping);

	// expand the grouping sets into a union all operator
	gpos::Ref<CDXLNode> CreateDXLUnionAllForGroupingSets(
		FromExpr *from_expr, List *target_list, List *group_clause,
		BOOL has_aggs, gpos::Ref<CBitSetArray> pdrgpbsGroupingSets,
		IntToUlongMap *phmiulSortGrpColsColId,
		IntToUlongMap *output_attno_to_colid_mapping,
		UlongToUlongMap *
			grpcol_index_to_colid_mapping  // mapping pos->unique grouping columns for grouping func arguments
	);

	// construct a project node with NULL values for columns not included in the grouping set
	gpos::Ref<CDXLNode> CreateDXLProjectNullsForGroupingSets(
		List *target_list, gpos::Ref<CDXLNode> child_dxlnode,
		gpos::Ref<CBitSet> bitset, IntToUlongMap *sort_grouping_col_mapping,
		IntToUlongMap *output_attno_to_colid_mapping,
		UlongToUlongMap *grpcol_index_to_colid_mapping) const;

	// construct a project node with appropriate values for the grouping funcs in the given target list
	gpos::Ref<CDXLNode> CreateDXLProjectGroupingFuncs(
		List *target_list, gpos::Ref<CDXLNode> child_dxlnode,
		gpos::Ref<CBitSet> bitset, IntToUlongMap *output_attno_to_colid_mapping,
		UlongToUlongMap *grpcol_index_to_colid_mapping,
		IntToUlongMap *sort_grpref_to_colid_mapping) const;

	// add sorting and grouping column into the hash map
	void AddSortingGroupingColumn(TargetEntry *target_entry,
								  IntToUlongMap *phmiulSortGrpColsColId,
								  ULONG colid) const;

	// translate the list of sorting columns
	gpos::Ref<CDXLNodeArray> TranslateSortColumsToDXL(
		List *sort_clause, IntToUlongMap *col_attno_colid_mapping) const;

	// translate the list of partition-by column identifiers
	gpos::Ref<ULongPtrArray> TranslatePartColumns(
		List *sort_clause, IntToUlongMap *col_attno_colid_mapping) const;

	gpos::Ref<CDXLNode> TranslateLimitToDXLGroupBy(
		List *plsortcl,			  // list of sort clauses
		Node *limit_count,		  // query node representing the limit count
		Node *limit_offset_node,  // query node representing the limit offset
		gpos::Ref<CDXLNode> dxlnode,  // the dxl node representing the subtree
		IntToUlongMap *
			grpcols_to_colid_mapping  // the mapping between the position in the TargetList to the ColId
	);

	// throws an exception when RTE kind not yet supported
	[[noreturn]] static void UnsupportedRTEKind(RTEKind rtekind);

	// translate an entry of the from clause (this can either be FromExpr or JoinExpr)
	gpos::Ref<CDXLNode> TranslateFromClauseToDXL(Node *node);

	// translate the target list entries of the query into a logical project
	gpos::Ref<CDXLNode> TranslateTargetListToDXLProject(
		List *target_list, gpos::Ref<CDXLNode> child_dxlnode,
		IntToUlongMap *group_col_to_colid_mapping,
		IntToUlongMap *output_attno_to_colid_mapping, List *group_clause,
		BOOL is_aggref_expanded = false);

	// translate a target list entry or a join alias entry into a project element
	gpos::Ref<CDXLNode> TranslateExprToDXLProject(
		Expr *expr, const CHAR *alias_name, BOOL insist_new_colids = false);

	// translate a CTE into a DXL logical CTE operator
	gpos::Ref<CDXLNode> TranslateCTEToDXL(const RangeTblEntry *rte, ULONG rti,
										  ULONG current_query_level);

	// translate a base table range table entry into a logical get
	gpos::Ref<CDXLNode> TranslateRTEToDXLLogicalGet(const RangeTblEntry *rte,
													ULONG rti,
													ULONG  //current_query_level
	);

	void NoteDistributionPolicyOpclasses(const RangeTblEntry *rte);

	// generate a DXL node from column values, where each column value is
	// either a datum or scalar expression represented as a project element.
	gpos::Ref<CDXLNode> TranslateColumnValuesToDXL(
		CDXLDatumArray *dxl_datum_array,
		CDXLColDescrArray *dxl_column_descriptors,
		CDXLNodeArray *dxl_project_elements) const;

	// translate a value scan range table entry
	gpos::Ref<CDXLNode> TranslateValueScanRTEToDXL(const RangeTblEntry *rte,
												   ULONG rti,
												   ULONG  //current_query_level
	);

	// create a dxl node from a array of datums and project elements
	gpos::Ref<CDXLNode> TranslateTVFToDXL(const RangeTblEntry *rte, ULONG rti,
										  ULONG	 //current_query_level
	);

	// translate a derived table into a DXL logical operator
	gpos::Ref<CDXLNode> TranslateDerivedTablesToDXL(const RangeTblEntry *rte,
													ULONG rti,
													ULONG current_query_level);

	// create a DXL node representing the scalar constant "true"
	gpos::Ref<CDXLNode> CreateDXLConstValueTrue();

	// store mapping attno->colid
	void StoreAttnoColIdMapping(IntToUlongMap *attno_to_colid_mapping,
								INT attno, ULONG colid) const;

	// construct an array of output columns
	gpos::Ref<CDXLNodeArray> CreateDXLOutputCols(
		List *target_list, IntToUlongMap *attno_to_colid_mapping) const;

	// check for support command types, throws an exception when command type not yet supported
	static void CheckSupportedCmdType(Query *query);

	// check for supported range table entries, throws an exception when something is not yet supported
	static void CheckRangeTable(Query *query);

	// translate a select-project-join expression into DXL
	gpos::Ref<CDXLNode> TranslateSelectProjectJoinToDXL(
		List *target_list, FromExpr *from_expr,
		IntToUlongMap *sort_group_attno_to_colid_mapping,
		IntToUlongMap *output_attno_to_colid_mapping, List *group_clause);

	// translate a select-project-join expression into DXL and keep variables appearing
	// in aggregates and grouping columns in the output column map
	gpos::Ref<CDXLNode> TranslateSelectProjectJoinForGrpSetsToDXL(
		List *target_list, FromExpr *from_expr,
		IntToUlongMap *sort_group_attno_to_colid_mapping,
		IntToUlongMap *output_attno_to_colid_mapping, List *group_clause);

	// helper to check if OID is included in given array of OIDs
	static BOOL OIDFound(OID oid, const OID oids[], ULONG size);

	// check if given operator is lead() window function
	static BOOL IsLeadWindowFunc(gpos::Ref<CDXLOperator> dxlop);

	// check if given operator is lag() window function
	static BOOL IsLagWindowFunc(gpos::Ref<CDXLOperator> dxlop);

	// translate an insert query
	gpos::Ref<CDXLNode> TranslateInsertQueryToDXL();

	// translate a delete query
	gpos::Ref<CDXLNode> TranslateDeleteQueryToDXL();

	// translate an update query
	gpos::Ref<CDXLNode> TranslateUpdateQueryToDXL();

	// translate a CTAS query
	gpos::Ref<CDXLNode> TranslateCTASToDXL();
	// translate CTAS storage options
	gpos::Ref<CDXLCtasStorageOptions::CDXLCtasOptionArray>
	GetDXLCtasOptionArray(List *options,
						  IMDRelation::Erelstoragetype *storage_type);

	// extract storage option value from defelem
	CWStringDynamic *ExtractStorageOptionStr(DefElem *def_elem);

	// return resno -> colId mapping of columns to be updated
	gpos::Ref<IntToUlongMap> UpdatedColumnMapping();

	// obtain the ids of the ctid and segmentid columns for the target
	// table of a DML query
	void GetCtidAndSegmentId(ULONG *ctid, ULONG *segment_id);

	// translate a grouping func expression
	gpos::Ref<CDXLNode> TranslateGroupingFuncToDXL(
		const Expr *expr, gpos::Ref<CBitSet> bitset,
		UlongToUlongMap *grpcol_index_to_colid_mapping) const;

	// construct a list of CTE producers from the query's CTE list
	void ConstructCTEProducerList(List *cte_list, ULONG query_level);

	// construct a stack of CTE anchors for each CTE producer in the given array
	void ConstructCTEAnchors(CDXLNodeArray *dxlnodes,
							 gpos::Ref<CDXLNode> *dxl_cte_anchor_top,
							 gpos::Ref<CDXLNode> *dxl_cte_anchor_bottom);

	// generate an array of new column ids of the given size
	gpos::Ref<ULongPtrArray> GenerateColIds(CMemoryPool *mp, ULONG size) const;

	// extract an array of colids from the given column mapping
	gpos::Ref<ULongPtrArray> ExtractColIds(
		CMemoryPool *mp, IntToUlongMap *attno_to_colid_mapping) const;

	// construct a new mapping based on the given one by replacing the colid in the "From" list
	// with the colid at the same position in the "To" list
	static gpos::Ref<IntToUlongMap> RemapColIds(
		CMemoryPool *mp, IntToUlongMap *attno_to_colid_mapping,
		ULongPtrArray *from_list_colids, ULongPtrArray *to_list_colids);

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
	gpos::Ref<CDXLNode> TranslateSelectQueryToDXL();

	// main driver
	gpos::Ref<CDXLNode> TranslateQueryToDXL();

	// return the list of output columns
	CDXLNodeArray *GetQueryOutputCols() const;

	// return the list of CTEs
	CDXLNodeArray *GetCTEs() const;

	// factory function
	static CTranslatorQueryToDXL *QueryToDXLInstance(CMemoryPool *mp,
													 CMDAccessor *md_accessor,
													 Query *query);
};
}  // namespace gpdxl
#endif	// GPDXL_CTranslatorQueryToDXL_H

//EOF
