//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (C) 2019 VMware, Inc. or its affiliates.
//
//	@filename:
//		CJoinOrderDPv2.h
//
//	@doc:
//		Dynamic programming-based join order generation
//---------------------------------------------------------------------------
#ifndef GPOPT_CJoinOrderDPv2_H
#define GPOPT_CJoinOrderDPv2_H

#include "gpos/base.h"
#include "gpos/common/CBitSet.h"
#include "gpos/common/CHashMap.h"
#include "gpos/common/DbgPrintMixin.h"
#include "gpos/io/IOstream.h"

#include "gpopt/base/CKHeap.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/xforms/CJoinOrder.h"


namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CJoinOrderDPv2
//
//	@doc:
//		Helper class for creating join orders using dynamic programming
//
//		Some terminology:
//
//		NIJ:	Non-inner join. This is sometimes used instead of left join,
//				since we anticipate to extend this code to semi-joins and other
//				types of joins.
//		Atom:	A child of the NAry join. This could be a table or some
//				other operator like a groupby or a full outer join.
//		Group:	A set of atoms (called a "component" in DPv1). Once we generate
//				a result expression, each of these sets will be associated
//				with a CGroup in MEMO.
//---------------------------------------------------------------------------
class CJoinOrderDPv2 : public CJoinOrder,
					   public gpos::DbgPrintMixin<CJoinOrderDPv2>
{
private:
	// Data structures for DPv2 join enumeration:
	//
	// Each level l is the set of l-way joins we are considering.
	// Level 1 describes the "atoms" the leaves of the original NAry join we are transforming.
	//
	// Each level consists of a set (an array) of "groups". A group represents what may eventually
	// become a group in the MEMO structure, if it is a part of one of the generated top k
	// expressions at the top level. It is a set of atoms to be joined (in any order).
	// The SGroupInfo struct contains the bitset representing the atoms, the cardinality from the
	// derived statistics and an array of SExpressionInfo structs.
	//
	// Each SExpressionInfo struct describes an expression in a group. Besides a CExpression,
	// it also has the SGroupInfo and SExpressionInfo of the children. Each SExpressionInfo
	// also has a property. The SExpressionInfo entries in a group all have different properties.
	// We only keep expressions that are not dominated by another expression, meaning that there
	// is no other expression that can produce a superset of the properties for a less or equal
	// cost.
	//
	//
	//           SLevelInfo
	//           +---------------------+
	// Level n:  | SGroupInfo array ---+----> SGroupInfo
	//           | optional top k      |      +------------------+
	//           +---------------------+      | Atoms (bitset)   |
	//                                        | cardinality      |
	//                                        | SExpressionInfo  |
	//                                        |      array       |
	//                                        +--------+---------+
	//                                                 v
	//                                          SExpressionInfo
	//                                          +------------------------+
	//                                          | CExpression            |
	//                                          | child SExpressionInfos |
	//                                          | properties             |
	//                                          +------------------------+
	//                                          +------------------------+
	//                                          | CExpression            |
	//                                          | child SExpressionInfos |
	//                                          | properties             |
	//                                          +------------------------+
	//                                           ...
	//                                          +------------------------+
	//                                          | CExpression            |
	//                                          | child SExpressionInfos |
	//                                          | properties             |
	//                                          +------------------------+
	//           ...
	//
	//           SLevelInfo
	//           +---------------------+
	// Level 1:  | SGroupInfo array ---+----> SGroupInfo                        SGroupInfo
	//           | optional top k      |      +------------------+              +------------------+
	//           +---------------------+      | Atoms (bitset)   |              | Atoms (bitset)   |
	//                                        | cardinality      +--------------+ cardinality      |
	//                                        | ExpressionInfo   |              | ExpressionInfo   |
	//                                        |      array       |              |      array       |
	//                                        +--------+---------+              +--------+---------+
	//                                                 v                                 v
	//                                          SExpressionInfo                   SExpressionInfo
	//                                          +------------------------+        +------------------------+
	//                                          | CExpression            |        | CExpression            |
	//                                          | child SExpressionInfos |        | child SExpressionInfos |
	//                                          | properties             |        | properties             |
	//                                          +------------------------+        +------------------------+
	//

	// forward declarations, circular reference
	struct SGroupInfo;
	struct SExpressionInfo;

	// Join enumeration algorithm properties, these can be added if an expression satisfies more than one
	// consider these as constants, not as a true enum
	// note that the numbers (other than the first) must be powers of 2,
	// since we add them to make composite properties!!!
	// Note also that query, mincard and GreedyAvoidXProd are all greedy algorithms.
	// Sorry for the confusion with the term "greedy" used in the optimizer_join_order guc
	// and the CXformExpandNAryJoinGreedy classes, where they refer to one type of greedy
	// algorithm that avoids cross products.
	enum JoinOrderPropType
	{
		EJoinOrderAny = 0,	// the overall best solution (used for exhaustive2)
		EJoinOrderQuery = 1,	// this expression uses the "query" join order
		EJoinOrderMincard = 2,	// this expression has the "mincard" property
		EJoinOrderGreedyAvoidXProd =
			4,	// best "greedy" expression with minimal cross products
		EJoinOrderHasPS =
			8,	// best expression with special consideration for DPE
		EJoinOrderDP = 16,	// best solution using DP
		EJoinOrderStats =
			32	// this expression is used to calculate the statistics
				// (row count) for the group
	};

	// properties of an expression in the DP structure (also used as required properties)
	struct SExpressionProperties
	{
		// the join order enumeration algorithm for which this is a solution
		// (exhaustive enumeration, can use any of these: EJoinOrderAny)
		ULONG m_join_order;

		SExpressionProperties(ULONG join_order_properties)
			: m_join_order(join_order_properties)
		{
		}

		BOOL
		Satisfies(ULONG pt) const
		{
			return pt == (m_join_order & pt);
		}
		void
		Add(const SExpressionProperties &p)
		{
			m_join_order |= p.m_join_order;
		}
		BOOL
		IsGreedy() const
		{
			return 0 != (m_join_order & (EJoinOrderQuery + EJoinOrderMincard +
										 EJoinOrderGreedyAvoidXProd));
		}
	};

	// a simple wrapper of an gpos::Ref<SGroupInfo>  plus an index into its array of SExpressionInfos
	// this identifies a group and one expression belonging to that group
	struct SGroupAndExpression
	{
		gpos::Ref<SGroupInfo> m_group_info{nullptr};
		ULONG m_expr_index{gpos::ulong_max};

		SGroupAndExpression() = default;
		SGroupAndExpression(gpos::Ref<SGroupInfo> g, ULONG ix)
			: m_group_info(g), m_expr_index(ix)
		{
		}
		gpos::Ref<SExpressionInfo>
		GetExprInfo() const
		{
			return m_expr_index == gpos::ulong_max
					   ? nullptr
					   : (*m_group_info->m_best_expr_info_array)[m_expr_index];
		}
		BOOL
		IsValid() const
		{
			return nullptr != m_group_info && gpos::ulong_max != m_expr_index;
		}
		BOOL
		operator==(const SGroupAndExpression &other) const
		{
			return m_group_info == other.m_group_info &&
				   m_expr_index == other.m_expr_index;
		}
	};

	// description of an expression in the DP environment
	// left and right child of join expressions point to
	// child groups + expressions
	struct SExpressionInfo : public CRefCount<SExpressionInfo>
	{
		// the expression
		gpos::Ref<CExpression> m_expr;

		// left/right child group/expr info (group for left/right child of m_expr),
		// we do not keep a refcount for these
		SGroupAndExpression m_left_child_expr;
		SGroupAndExpression m_right_child_expr;
		// derived properties of this expression
		SExpressionProperties m_properties;

		// in the future, we may add more properties relevant to the cost here,
		// like distribution spec, partition selectors

		// Stores part keys for atoms that are partitioned tables. NULL otherwise.
		CPartKeysArray *m_atom_part_keys_array;

		// cost of the expression
		CDouble m_cost;

		//cost adjustment for the effect of partition selectors, this is always <= 0.0
		CDouble m_cost_adj_PS;

		// base table rows, -1 if not atom or get/select
		CDouble m_atom_base_table_rows;

		// stores atom ids that are fufilled by a PS in this expression
		gpos::Ref<CBitSet> m_contain_PS;

		SExpressionInfo(CMemoryPool *mp, gpos::Ref<CExpression> expr,
						const SGroupAndExpression &left_child_expr_info,
						const SGroupAndExpression &right_child_expr_info,
						SExpressionProperties &properties)
			: m_expr(expr),
			  m_left_child_expr(left_child_expr_info),
			  m_right_child_expr(right_child_expr_info),
			  m_properties(properties),
			  m_atom_part_keys_array(nullptr),
			  m_cost(0.0),
			  m_cost_adj_PS(0.0),
			  m_atom_base_table_rows(-1.0),
			  m_contain_PS(nullptr)

		{
			m_contain_PS = GPOS_NEW(mp) CBitSet(mp);
			this->UnionPSProperties(left_child_expr_info.GetExprInfo());
			this->UnionPSProperties(right_child_expr_info.GetExprInfo());
		}

		SExpressionInfo(CMemoryPool *mp, gpos::Ref<CExpression> expr,
						SExpressionProperties &properties)
			: m_expr(expr),
			  m_properties(properties),
			  m_atom_part_keys_array(nullptr),
			  m_cost(0.0),
			  m_cost_adj_PS(0.0),
			  m_atom_base_table_rows(-1.0),
			  m_contain_PS(nullptr)
		{
			m_contain_PS = GPOS_NEW(mp) CBitSet(mp);
		}

		~SExpressionInfo() override
		{
			m_expr->Release();
			SafeRelease(m_contain_PS);
		}

		// cost (use -1 for greedy solutions to ensure we keep all of them)
		CDouble
		GetCostForHeap() const
		{
			return m_properties.IsGreedy() ? -1.0 : GetCost();
		}

		CDouble
		GetCost() const
		{
			return m_cost + m_cost_adj_PS;
		}

		void
		UnionPSProperties(gpos::Ref<SExpressionInfo> other) const
		{
			m_contain_PS->Union(other->m_contain_PS);
		}
		BOOL
		ChildrenAreEqual(const SExpressionInfo &other) const
		{
			return m_left_child_expr == other.m_left_child_expr &&
				   m_right_child_expr == other.m_right_child_expr;
		}
	};

	using SExpressionInfoArray =
		CDynamicPtrArray<SExpressionInfo, CleanupRelease<SExpressionInfo>>;

	//---------------------------------------------------------------------------
	//	@struct:
	//		SGroupInfo
	//
	//	@doc:
	//		Struct containing a bitset, representing a group, its best expression, and cost
	//
	//---------------------------------------------------------------------------
	struct SGroupInfo : public CRefCount<SGroupInfo>
	{
		// the set of atoms, this uniquely identifies the group
		gpos::Ref<CBitSet> m_atoms;
		// infos of the best (lowest cost) expressions (so far, if at the current level)
		// for each interesting property
		SExpressionInfoArray *m_best_expr_info_array;
		CDouble m_cardinality;
		CDouble m_lowest_expr_cost;

		SGroupInfo(CMemoryPool *mp, gpos::Ref<CBitSet> atoms)
			: m_atoms(atoms), m_cardinality(-1.0), m_lowest_expr_cost(-1.0)
		{
			m_best_expr_info_array = GPOS_NEW(mp) SExpressionInfoArray(mp);
		}

		~SGroupInfo() override
		{
			m_atoms->Release();
			m_best_expr_info_array->Release();
		}

		BOOL
		IsAnAtom() const
		{
			return 1 == m_atoms->Size();
		}
		CDouble
		GetCostForHeap() const
		{
			return m_lowest_expr_cost;
		}
	};

	// dynamic array of SGroupInfo, where each index represents an alternative group of a given level k
	using SGroupInfoArray =
		CDynamicPtrArray<SGroupInfo, CleanupRelease<SGroupInfo>>;

	// info for a join level, the set of all groups representing <m_level>-way joins
	struct SLevelInfo : public CRefCount<SLevelInfo>
	{
		ULONG m_level;
		SGroupInfoArray *m_groups;
		CKHeap<SGroupInfoArray, SGroupInfo> *m_top_k_groups;

		SLevelInfo(ULONG level, SGroupInfoArray *groups)
			: m_level(level), m_groups(groups), m_top_k_groups(nullptr)
		{
		}

		~SLevelInfo() override
		{
			m_groups->Release();
			SafeRelease(m_top_k_groups);
		}
	};

	// hashing function
	static ULONG
	UlHashBitSet(const gpos::Ref<CBitSet> pbs)
	{
		GPOS_ASSERT(nullptr != pbs);

		return pbs->HashValue();
	}

	// equality function
	static BOOL
	FEqualBitSet(const gpos::Ref<CBitSet> pbsFst,
				 const gpos::Ref<CBitSet> pbsSnd)
	{
		GPOS_ASSERT(nullptr != pbsFst);
		GPOS_ASSERT(nullptr != pbsSnd);

		return pbsFst->Equals(pbsSnd);
	}

	using ExpressionToEdgeMap =
		CHashMap<CExpression, SEdge, CExpression::HashValue, CUtils::Equals,
				 CleanupRelease<CExpression>, CleanupRelease<SEdge>>;

	// dynamic array of SGroupInfos
	using BitSetToGroupInfoMap =
		CHashMap<CBitSet, SGroupInfo, UlHashBitSet, FEqualBitSet,
				 CleanupRelease<CBitSet>, CleanupRelease<SGroupInfo>>;

	// iterator over group infos in a level
	using BitSetToGroupInfoMapIter =
		CHashMapIter<CBitSet, SGroupInfo, UlHashBitSet, FEqualBitSet,
					 CleanupRelease<CBitSet>, CleanupRelease<SGroupInfo>>;

	// dynamic array of SLevelInfos, where each index represents the level
	using DPv2Levels = CDynamicPtrArray<SLevelInfo, CleanupRelease<SLevelInfo>>;

	// an array of an array of groups, organized by level at the first array dimension,
	// main data structure for dynamic programming
	DPv2Levels *m_join_levels;

	// map to find the associated edge in the join graph from a join predicate
	ExpressionToEdgeMap *m_expression_to_edge_map;

	// map to check whether a DPv2 group already exists
	BitSetToGroupInfoMap *m_bitset_to_group_info_map;

	// ON predicates for NIJs (non-inner joins, e.g. LOJs)
	// currently NIJs are LOJs only, this may change in the future
	// if/when we add semijoins, anti-semijoins and relatives
	CExpressionArray *m_on_pred_conjuncts;

	// association between logical children and inner join/ON preds
	// (which of the logical children are right children of NIJs and what ON predicates are they using)
	ULongPtrArray *m_child_pred_indexes;

	// for each non-inner join (entry in m_on_pred_conjuncts), the required atoms on the left
	CBitSetArray *m_non_inner_join_dependencies;

	// top K expressions at the top level
	CKHeap<SExpressionInfoArray, SExpressionInfo> *m_top_k_expressions;

	// top K expressions at top level that contain promising dynamic partiion selectors
	// if there are no promising dynamic partition selectors, this will be empty
	CKHeap<SExpressionInfoArray, SExpressionInfo> *m_top_k_part_expressions;

	// current penalty for cross products (depends on enumeration algorithm)
	CDouble m_cross_prod_penalty;

	// outer references, if any
	gpos::Ref<CColRefSet> m_outer_refs;

	CMemoryPool *m_mp;

	gpos::Ref<SLevelInfo>
	Level(ULONG l)
	{
		return (*m_join_levels)[l];
	}

	// build expression linking given groups
	gpos::Ref<CExpression> PexprBuildInnerJoinPred(gpos::Ref<CBitSet> pbsFst,
												   gpos::Ref<CBitSet> pbsSnd);

	// compute cost of a join expression in a group
	void ComputeCost(gpos::Ref<SExpressionInfo> expr_info,
					 CDouble join_cardinality);

	// if we need to keep track of used edges, make a map that
	// speeds up this usage check
	void PopulateExpressionToEdgeMapIfNeeded();

	// add a select node with any remaining edges (predicates) that have
	// not been incorporated in the join tree
	gpos::Ref<CExpression> AddSelectNodeForRemainingEdges(
		gpos::Ref<CExpression> join_expr);

	// mark all the edges used in a join tree
	void RecursivelyMarkEdgesAsUsed(gpos::Ref<CExpression> expr);

	// enumerate all possible joins between left_level-way joins on the left side
	// and right_level-way joins on the right side, resulting in left_level + right_level-way joins
	void SearchJoinOrders(ULONG left_level, ULONG right_level);

	void GreedySearchJoinOrders(ULONG left_level, JoinOrderPropType algo);

	void DeriveStats(gpos::Ref<CExpression> pexpr) override;

	// create a CLogicalJoin and a CExpression to join two groups, for a required property
	gpos::Ref<SExpressionInfo> GetJoinExprForProperties(
		gpos::Ref<SGroupInfo> left_child, gpos::Ref<SGroupInfo> right_child,
		SExpressionProperties &required_properties);

	// get a join expression from two child groups with specified child expressions
	gpos::Ref<SExpressionInfo> GetJoinExpr(
		const SGroupAndExpression &left_child_expr,
		const SGroupAndExpression &right_child_expr,
		SExpressionProperties &result_properties);

	// does "prop" provide all the properties of "other_prop" plus maybe more?
	static BOOL IsASupersetOfProperties(SExpressionProperties &prop,
										SExpressionProperties &other_prop);

	// is one of the properties a subset of the other or are they disjoint?
	static BOOL ArePropertiesDisjoint(SExpressionProperties &prop,
									  SExpressionProperties &other_prop);

	// get best expression in a group for a given set of properties
	static SGroupAndExpression GetBestExprForProperties(
		gpos::Ref<SGroupInfo> group_info, SExpressionProperties &props);

	// add a new property to an existing predicate
	static void AddNewPropertyToExpr(gpos::Ref<SExpressionInfo> expr_info,
									 SExpressionProperties props);

	// enumerate bushy joins (joins where both children are also joins) of level "current_level"
	void SearchBushyJoinOrders(ULONG current_level);

	// look up an existing group or create a new one, with an expression to be used for stats
	gpos::Ref<SGroupInfo> LookupOrCreateGroupInfo(
		gpos::Ref<SLevelInfo> levelInfo, gpos::Ref<CBitSet> atoms,
		gpos::Ref<SExpressionInfo> stats_expr_info);
	// add a new expression to a group, unless there already is an existing expression that dominates it
	void AddExprToGroupIfNecessary(gpos::Ref<SGroupInfo> group_info,
								   gpos::Ref<SExpressionInfo> new_expr_info);

	void PopulateDPEInfo(gpos::Ref<SExpressionInfo> join_expr_info,
						 gpos::Ref<SGroupInfo> left_group_info,
						 gpos::Ref<SGroupInfo> right_group_info);

	void FinalizeDPLevel(ULONG level);

	SGroupInfoArray *
	GetGroupsForLevel(ULONG level) const
	{
		return (*m_join_levels)[level]->m_groups;
	}

	ULONG FindLogicalChildByNijId(ULONG nij_num);
	static ULONG NChooseK(ULONG n, ULONG k);
	BOOL LevelIsFull(ULONG level);

	void EnumerateDP();
	void EnumerateQuery();
	void FindLowestCardTwoWayJoin(JoinOrderPropType prop_type);
	void EnumerateMinCard();
	void EnumerateGreedyAvoidXProd();

public:
	// ctor
	CJoinOrderDPv2(CMemoryPool *mp, CExpressionArray *pdrgpexprAtoms,
				   CExpressionArray *innerJoinConjuncts,
				   CExpressionArray *onPredConjuncts,
				   ULongPtrArray *childPredIndexes,
				   gpos::Ref<CColRefSet> outerRefs);

	// dtor
	~CJoinOrderDPv2() override;

	// main handler
	virtual void PexprExpand();

	gpos::Ref<CExpression> GetNextOfTopK();

	// check for NIJs
	BOOL IsRightChildOfNIJ(gpos::Ref<SGroupInfo> groupInfo,
						   gpos::Ref<CExpression> *onPredToUse = nullptr,
						   gpos::Ref<CBitSet> *requiredBitsOnLeft = nullptr);

	// print function
	IOstream &OsPrint(IOstream &) const;

	static IOstream &OsPrintProperty(IOstream &, SExpressionProperties &);

	CXform::EXformId
	EOriginXForm() const override
	{
		return CXform::ExfExpandNAryJoinDPv2;
	}

};	// class CJoinOrderDPv2

}  // namespace gpopt

#endif	// !GPOPT_CJoinOrderDPv2_H

// EOF
