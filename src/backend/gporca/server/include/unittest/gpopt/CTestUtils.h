//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CTestUtils.h
//
//	@doc:
//		Optimizer test utility functions
//---------------------------------------------------------------------------
#ifndef GPOPT_CTestUtils_H
#define GPOPT_CTestUtils_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/base/CColRef.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CQueryContext.h"
#include "gpopt/base/CRange.h"
#include "gpopt/engine/CEnumeratorConfig.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CLogicalGet.h"
#include "gpopt/operators/CScalarArrayCmp.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "naucrates/md/CMDTypeInt2GPDB.h"
#include "naucrates/md/CMDTypeInt4GPDB.h"
#include "naucrates/md/CMDTypeInt8GPDB.h"
#include "naucrates/md/CSystemId.h"
#include "naucrates/statistics/CPoint.h"

#include "unittest/base.h"


#define GPOPT_MDCACHE_TEST_OID 1234
#define GPOPT_MDCACHE_TEST_OID_PARTITIONED 5678
#define GPOPT_MDCACHE_TEST_OID_PARTITIONED_WITH_INDEXES 5369655
#define GPOPT_TEST_REL_WIDTH 3	// number of columns in generated test relations
#define GPOPT_TEST_PART_INDEX 1234567

#define GPOPT_TEST_REL_OID1 OID(11111)
#define GPOPT_TEST_REL_OID2 OID(22222)
#define GPOPT_TEST_REL_OID3 OID(33333)
#define GPOPT_TEST_REL_OID4 OID(44444)
#define GPOPT_TEST_REL_OID5 OID(55555)
#define GPOPT_TEST_REL_OID6 OID(66666)
#define GPOPT_TEST_REL_OID7 OID(77777)
#define GPOPT_TEST_REL_OID8 OID(88888)
#define GPOPT_TEST_REL_OID9 OID(99999)
#define GPOPT_TEST_REL_OID10 OID(10000)
#define GPOPT_TEST_REL_OID11 OID(10111)
#define GPOPT_TEST_REL_OID12 OID(10222)
#define GPOPT_TEST_REL_OID13 OID(10333)
#define GPOPT_TEST_REL_OID14 OID(10444)
#define GPOPT_TEST_REL_OID15 OID(10555)
#define GPOPT_TEST_REL_OID16 OID(10666)
#define GPOPT_TEST_REL_OID17 OID(10777)
#define GPOPT_TEST_REL_OID18 OID(10888)
#define GPOPT_TEST_REL_OID19 OID(10999)
#define GPOPT_TEST_REL_OID20 OID(10200)
#define GPOPT_TEST_REL_OID21 OID(10311)
#define GPOPT_TEST_REL_OID22 OID(27118)

#define GPDB_INT4_LT_OP OID(97)
#define GPDB_INT4_EQ_OP OID(96)
#define GPDB_INT4_NEQ_OP OID(518)
#define GPDB_INT4_SUM_AGG OID(2114)
#define GPDB_INT8_GENERATE_SERIES OID(1067)

// forward declarations
namespace gpmd
{
class CMDIdGPDB;
}

namespace gpopt
{
using namespace gpos;
using gpmd::CMDIdGPDB;

// forward declarations
class IConstExprEvaluator;

using CExpressionJoinsArray = CDynamicPtrArray<CExpression, CleanupNULL>;

using FnDXLPlanChecker = BOOL(gpos::pointer<CDXLNode *>);

//---------------------------------------------------------------------------
//	@class:
//		CTestUtils
//
//	@doc:
//		Test utility functions
//
//---------------------------------------------------------------------------
class CTestUtils
{
private:
	// local memory pool
	static CMemoryPool *m_mp;

	// construct an array of segment ids
	static gpos::owner<gpdxl::IntPtrArray *> PdrgpiSegments(CMemoryPool *mp);

	// generate minidump file name from passed file name
	static CHAR *SzMinidumpFileName(CMemoryPool *mp, const CHAR *file_name);

public:
	// pair of DXL query file and the corresponding DXL plan file
	struct STestCase
	{
		const CHAR *szInputFile;
		const CHAR *szOutputFile;
	};

	//---------------------------------------------------------------------------
	//	@class:
	//		CTestUtils
	//
	//	@doc:
	//		Set up basic structures needed for most tests
	//
	//---------------------------------------------------------------------------
	class CTestSetup
	{
	private:
		// memory pool
		CAutoMemoryPool m_amp;

		// metadata accessor
		CMDAccessor m_mda;

		// optimization context
		CAutoOptCtxt m_aoc;

		// set up a file based provider
		static gpos::owner<CMDProviderMemory *> PmdpSetupFileBasedProvider();

	public:
		// private ctor
		CTestSetup(const CTestSetup &) = delete;

		// ctor
		CTestSetup();

		// return the memory pool
		CMemoryPool *
		Pmp()
		{
			return m_amp.Pmp();
		}

		// return the metadata acccessor
		CMDAccessor *
		Pmda()
		{
			return &m_mda;
		}
	};


	//-------------------------------------------------------------------
	// Initialization/Destruction of provider file
	//-------------------------------------------------------------------
	// initialize provider file
	static void InitProviderFile(CMemoryPool *mp);

	// destroy metadata provider
	static void DestroyMDProvider();

	//-------------------------------------------------------------------
	// Helpers for generating expressions
	//-------------------------------------------------------------------

	// generate a plain table descriptor
	static gpos::owner<CTableDescriptor *> PtabdescPlain(
		CMemoryPool *mp, ULONG num_cols, gpos::owner<IMDId *> mdid,
		const CName &nameTable, BOOL is_nullable = false);

	// generate a plain table descriptor, where the column names are generated using a format string containing %d
	static gpos::owner<CTableDescriptor *> PtabdescPlainWithColNameFormat(
		CMemoryPool *mp, ULONG num_cols, gpos::owner<IMDId *> mdid,
		const WCHAR *wszColNameFormat, const CName &nameTable,
		BOOL is_nullable = false);

	// generate a table descriptor
	static gpos::owner<CTableDescriptor *> PtabdescCreate(
		CMemoryPool *mp, ULONG num_cols, gpos::owner<IMDId *> mdid,
		const CName &name, BOOL fPartitioned = false);

	// generate a get expression
	static gpos::owner<CExpression *> PexprLogicalGet(
		CMemoryPool *mp, gpos::owner<CTableDescriptor *> ptabdesc,
		const CWStringConst *pstrTableAlias);

	// generate a get expression over table with nullable columns
	static gpos::owner<CExpression *> PexprLogicalGetNullable(
		CMemoryPool *mp, OID oidTable, const CWStringConst *str_table_name,
		const CWStringConst *pstrTableAlias);

	// generate a get expression
	static gpos::owner<CExpression *> PexprLogicalGet(
		CMemoryPool *mp, CWStringConst *str_table_name,
		CWStringConst *pstrTableAlias, ULONG ulTableId);

	// generate a random get expression
	static gpos::owner<CExpression *> PexprLogicalGet(CMemoryPool *mp);

	// generate a random external get expression
	static gpos::owner<CExpression *> PexprLogicalExternalGet(CMemoryPool *mp);

	// generate a random assert expression
	static gpos::owner<CExpression *> PexprLogicalAssert(CMemoryPool *mp);

	// generate a random get expression on a partitioned table
	static gpos::owner<CExpression *> PexprLogicalGetPartitioned(
		CMemoryPool *mp);

	// generate a randomized get expression for a partitioned table with indexes
	static gpos::owner<CExpression *> PexprLogicalDynamicGetWithIndexes(
		CMemoryPool *mp);

	// generate a select expression with random equality predicate
	static gpos::owner<CExpression *> PexprLogicalSelect(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr);

	// generate a select expression
	static gpos::owner<CExpression *> PexprLogicalSelect(
		CMemoryPool *mp, CWStringConst *str_table_name,
		CWStringConst *pstrTableAlias, ULONG ulTableId);

	// generate a random select expression
	static gpos::owner<CExpression *> PexprLogicalSelect(CMemoryPool *mp);

	// generate a select expression with a contradiction
	static gpos::owner<CExpression *> PexprLogicalSelectWithContradiction(
		CMemoryPool *mp);

	// generate a select on top of a partitioned table get with a filter on the part key
	static gpos::owner<CExpression *> PexprLogicalSelectPartitioned(
		CMemoryPool *mp);

	// generate a project element with sum agg function
	static gpos::owner<CExpression *> PexprPrjElemWithSum(CMemoryPool *mp,
														  CColRef *colref);

	// generate an join of specific join type on  get with a predicate
	// on the part key with a partitioned table on the outer side
	template <class T>
	static gpos::owner<CExpression *> PexprJoinPartitionedOuter(
		CMemoryPool *mp);

	// generate a join of specific join type on  get with a predicate
	// on the part key with a partitioned table on the inner side
	template <class T>
	static gpos::owner<CExpression *> PexprJoinPartitionedInner(
		CMemoryPool *mp);

	// generate an join of specific join type on top of a partitioned
	// table get with a predicate on the part key
	template <class T>
	static gpos::owner<CExpression *> PexprJoinPartitioned(
		CMemoryPool *mp, BOOL fOuterPartitioned);

	// generate an 3-way join including a partitioned table
	static gpos::owner<CExpression *> Pexpr3WayJoinPartitioned(CMemoryPool *mp);

	// generate an 4-way join including a partitioned table
	static gpos::owner<CExpression *> Pexpr4WayJoinPartitioned(CMemoryPool *mp);

	// generate a random select expression with nested AND tree
	static gpos::owner<CExpression *> PexprLogicalSelectWithNestedAnd(
		CMemoryPool *mp);

	// generate a random select expression with nested OR tree
	static gpos::owner<CExpression *> PexprLogicalSelectWithNestedOr(
		CMemoryPool *mp);

	// generate a random select expression with an even number of nested NOT nodes
	static gpos::owner<CExpression *> PexprLogicalSelectWithEvenNestedNot(
		CMemoryPool *mp);

	// generate a random select expression with an odd number of nested NOT nodes
	static gpos::owner<CExpression *> PexprLogicalSelectWithOddNestedNot(
		CMemoryPool *mp);

	// generate a random select expression with nested AND-OR tree
	static gpos::owner<CExpression *> PexprLogicalSelectWithNestedAndOrNot(
		CMemoryPool *mp);

	// generate a Select expression with ANY/ALL subquery predicate over a const table get
	static gpos::owner<CExpression *> PexprLogicalSubqueryWithConstTableGet(
		CMemoryPool *mp, COperator::EOperatorId op_id);

	// generate a random select expression with constant ANY subquery
	static gpos::owner<CExpression *> PexprLogicalSelectWithConstAnySubquery(
		CMemoryPool *mp);

	// generate a random select expression with constant ALL subquery
	static gpos::owner<CExpression *> PexprLogicalSelectWithConstAllSubquery(
		CMemoryPool *mp);

	// generate a correlated select expression
	static gpos::owner<CExpression *> PexprLogicalSelectCorrelated(
		CMemoryPool *mp);

	// generate a correlated select expression
	static gpos::owner<CExpression *> PexprLogicalSelectCorrelated(
		CMemoryPool *mp, CColRefSet *outer_refs, ULONG ulLevel);

	// generate a select on top of outer join expression
	static gpos::owner<CExpression *> PexprLogicalSelectOnOuterJoin(
		CMemoryPool *mp);

	// generate a join expression with a random equality predicate
	template <class T>
	static gpos::owner<CExpression *> PexprLogicalJoin(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		gpos::owner<CExpression *> pexprRight);

	// generate a random binary join expression of specific join type
	template <class T>
	static gpos::owner<CExpression *> PexprLogicalJoin(CMemoryPool *mp);

	// generate join with correlation beneath it
	static gpos::owner<CExpression *> PexprLogicalJoinCorrelated(
		CMemoryPool *mp);

	// generate join with a partitioned and indexed inner child
	static gpos::owner<CExpression *>
	PexprLogicalJoinWithPartitionedAndIndexedInnerChild(CMemoryPool *mp);

	// generate a select expression with predicate between a scalar ident and a constant
	static gpos::owner<CExpression *> PexprLogicalSelectCmpToConst(
		CMemoryPool *mp);

	// generate a select expression with an array compare
	static gpos::owner<CExpression *> PexprLogicalSelectArrayCmp(
		CMemoryPool *mp);

	// generate a select expression with an array compare
	static gpos::owner<CExpression *> PexprLogicalSelectArrayCmp(
		CMemoryPool *mp, CScalarArrayCmp::EArrCmpType earrcmptype,
		IMDType::ECmpType ecmptype);

	// generate a select expression with an array compare
	static gpos::owner<CExpression *> PexprLogicalSelectArrayCmp(
		CMemoryPool *mp, CScalarArrayCmp::EArrCmpType earrcmptype,
		IMDType::ECmpType ecmptype,
		gpos::pointer<const IntPtrArray *> pdrgpiVals);

	// generate an n-ary join expression
	static gpos::owner<CExpression *> PexprLogicalNAryJoin(
		CMemoryPool *mp, gpos::owner<CExpressionArray *> pdrgpexpr);

	// generate an n-ary join expression using given array of relation names
	static gpos::owner<CExpression *> PexprLogicalNAryJoin(
		CMemoryPool *mp, CWStringConst *pstrRel, ULONG *pulRel, ULONG ulRels,
		BOOL fCrossProduct);

	// generate a random n-ary expression
	static gpos::owner<CExpression *> PexprLogicalNAryJoin(CMemoryPool *mp);

	// generate left outer join on top of n-ary join expression
	static gpos::owner<CExpression *> PexprLeftOuterJoinOnNAryJoin(
		CMemoryPool *mp);

	// generate right outer join expression
	static gpos::owner<CExpression *> PexprRightOuterJoin(CMemoryPool *mp);

	// generate n-ary join on top of left outer join expression
	static gpos::owner<CExpression *> PexprNAryJoinOnLeftOuterJoin(
		CMemoryPool *mp);

	// generate a project expression
	static gpos::owner<CExpression *> PexprLogicalProject(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr, CColRef *colref,
		CColRef *new_colref);

	// generate a random project expression
	static gpos::owner<CExpression *> PexprLogicalProject(CMemoryPool *mp);

	// generate a Project over GbAgg with correlation beneath it
	static gpos::owner<CExpression *> PexprLogicalProjectGbAggCorrelated(
		CMemoryPool *mp);

	// generate a limit expression
	static gpos::owner<CExpression *> PexprLogicalLimit(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr, LINT iStart,
		LINT iRows, BOOL fGlobal, BOOL fHasCount);

	// generate a random limit expression
	static gpos::owner<CExpression *> PexprLogicalLimit(CMemoryPool *mp);

	// generate a random group by expression
	static gpos::owner<CExpression *> PexprLogicalGbAgg(CMemoryPool *mp);

	// generate a random group by expression
	static gpos::owner<CExpression *> PexprLogicalGbAggWithInput(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprInput);

	// generate a random group by with a sum expression
	static gpos::owner<CExpression *> PexprLogicalGbAggWithSum(CMemoryPool *mp);

	// generate a random group by over join expression
	static gpos::owner<CExpression *> PexprLogicalGbAggOverJoin(
		CMemoryPool *mp);

	// generate GbAgg with correlation beneath it
	static gpos::owner<CExpression *> PexprLogicalGbAggCorrelated(
		CMemoryPool *mp);

	// generate a dedup group by over inner join expression
	static gpos::owner<CExpression *> PexprLogicalGbAggDedupOverInnerJoin(
		CMemoryPool *mp);

	// generate a top-level apply expression
	template <class T>
	static gpos::owner<CExpression *> PexprLogicalApply(CMemoryPool *mp);

	// generate a top-level apply expression with an outer reference
	template <class T>
	static gpos::owner<CExpression *> PexprLogicalApplyWithOuterRef(
		CMemoryPool *mp);

	// generate a const table get expression
	static gpos::owner<CExpression *> PexprConstTableGet(CMemoryPool *mp,
														 ULONG ulElements);

	// generate a const table get expression with 5 elements
	static gpos::owner<CExpression *> PexprConstTableGet5(CMemoryPool *mp);

	// generate a logical insert expression
	static gpos::owner<CExpression *> PexprLogicalInsert(CMemoryPool *mp);

	// generate a logical delete expression
	static gpos::owner<CExpression *> PexprLogicalDelete(CMemoryPool *mp);

	// generate a logical update expression
	static gpos::owner<CExpression *> PexprLogicalUpdate(CMemoryPool *mp);

	// generate a dynamic get expression
	static gpos::owner<CExpression *> PexprLogicalDynamicGet(
		CMemoryPool *mp, gpos::owner<CTableDescriptor *> ptabdesc,
		const CWStringConst *pstrTableAlias, ULONG ulPartIndex);

	// generate a dynamic get expression
	static gpos::owner<CExpression *> PexprLogicalDynamicGet(CMemoryPool *mp);

	// generate a select with an equality predicate over a dynamic get expression
	static gpos::owner<CExpression *>
	PexprLogicalSelectWithEqPredicateOverDynamicGet(CMemoryPool *mp);

	// generate a select with a less than predicate over a dynamic get expression
	static gpos::owner<CExpression *>
	PexprLogicalSelectWithLTPredicateOverDynamicGet(CMemoryPool *mp);

	// generate a logical table-valued function expression with 2 constant arguments
	static gpos::owner<CExpression *> PexprLogicalTVFTwoArgs(CMemoryPool *mp);

	// generate a logical table-valued function expression with 3 constant arguments
	static gpos::owner<CExpression *> PexprLogicalTVFThreeArgs(CMemoryPool *mp);

	// generate a logical table-valued function expression with no arguments
	static gpos::owner<CExpression *> PexprLogicalTVFNoArgs(CMemoryPool *mp);

	// generate a logical table-valued function expression
	static gpos::owner<CExpression *> PexprLogicalTVF(CMemoryPool *mp,
													  ULONG ulArgs);

	// generate a cte producer on top of a select
	static gpos::owner<CExpression *> PexprLogicalCTEProducerOverSelect(
		CMemoryPool *mp, ULONG ulCTEId);

	// generate a CTE producer on top of a select
	static gpos::owner<CExpression *> PexprLogicalCTEProducerOverSelect(
		CMemoryPool *mp);

	// generate an expression with CTE producer and consumer
	static gpos::owner<CExpression *> PexprCTETree(CMemoryPool *mp);

	// generate a logical sequence expression
	static gpos::owner<CExpression *> PexprLogicalSequence(
		CMemoryPool *mp, gpos::owner<CExpressionArray *> pdrgpexpr);

	// generate a logical sequence expression
	static gpos::owner<CExpression *> PexprLogicalSequence(CMemoryPool *mp);

	// generate a random union expression
	static gpos::owner<CExpression *> PexprLogicalUnion(CMemoryPool *mp,
														ULONG ulDepth);

	// generate a sequence project expression
	static gpos::owner<CExpression *> PexprLogicalSequenceProject(
		CMemoryPool *mp, OID oidFunc, gpos::owner<CExpression *> pexprInput);

	// generate a random expression with one window function
	static gpos::owner<CExpression *> PexprOneWindowFunction(CMemoryPool *mp);

	// generate a random expression with two window functions
	static gpos::owner<CExpression *> PexprTwoWindowFunctions(CMemoryPool *mp);

	// generate a dynamic array of join expressions
	static gpos::owner<CExpressionJoinsArray *> PdrgpexprJoins(
		CMemoryPool *mp, CWStringConst *pstrRel, ULONG *pulRel, ULONG ulRels,
		BOOL fCrossProduct);

	// generate a query context from an array of required column references
	static CQueryContext *PqcGenerate(CMemoryPool *mp, CExpression *pexpr,
									  CColRefArray *colref_array);

	// generate a dummy query context for testing
	static CQueryContext *PqcGenerate(CMemoryPool *mp, CExpression *pexpr);

	// generate a nested AND/OR tree
	static gpos::owner<CExpression *> PexprScalarNestedPreds(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		CScalarBoolOp::EBoolOperator eboolop);

	// find the first child expression with the given operator id
	static CExpression *PexprFindFirstExpressionWithOpId(
		CExpression *pexpr, COperator::EOperatorId op_id);

	// equality predicate shortcut
	static void EqualityPredicate(CMemoryPool *mp,
								  gpos::pointer<CColRefSet *> pcrsLeft,
								  gpos::pointer<CColRefSet *> pcrsRight,
								  gpos::pointer<CExpressionArray *> pdrgpexpr);

	// int2 point
	static gpos::owner<CPoint *> PpointInt2(CMemoryPool *mp, INT i);

	// int4 point
	static gpos::owner<CPoint *> PpointInt4(CMemoryPool *mp, INT i);

	// create an INT4 point with null value
	static gpos::owner<CPoint *> PpointInt4NullVal(CMemoryPool *mp);

	// int8 point
	static gpos::owner<CPoint *> PpointInt8(CMemoryPool *mp, INT li);

	// bool point
	static gpos::owner<CPoint *> PpointBool(CMemoryPool *mp, BOOL value);

	// perform DXL(query) -> CExpression(Log) ->CExpression(Ph) -> DXL(plan)
	// and check whether the resulting plan DXL matches the expected output
	static GPOS_RESULT EresTranslate(CMemoryPool *mp,
									 const CHAR *szQueryFileName,
									 const CHAR *szPlanFileName,
									 BOOL fIgnoreMismatch);

	// return query logical expression from minidump
	static gpos::owner<CExpression *> PexprReadQuery(
		CMemoryPool *mp, const CHAR *szQueryFileName);

	// compare expected and actual output
	static GPOS_RESULT EresCompare(IOstream &os, CWStringDynamic *pstrActual,
								   CWStringDynamic *pstrExpected,
								   BOOL fIgnoreMismatch = true);

	static CHAR *ExtractFilenameFromPath(CHAR *file_path);

	static void CreateExpectedAndActualFile(CMemoryPool *mp,
											const CHAR *file_name,
											CWStringDynamic *strExpected,
											CWStringDynamic *strActual);

	// match given two plans using string comparison
	static BOOL FPlanMatch(CMemoryPool *mp, IOstream &os,
						   gpos::pointer<const CDXLNode *> first_child_dxlnode,
						   ULLONG ullPlanIdFst, ULLONG ullPlanSpaceSizeFst,
						   gpos::pointer<const CDXLNode *> second_child_dxlnode,
						   ULLONG ullPlanIdSnd, ULLONG ullPlanSpaceSizeSnd,
						   const CHAR *file_name);

	// compare given DXL plans
	static BOOL FPlanCompare(
		CMemoryPool *mp, IOstream &os,
		gpos::pointer<const CDXLNode *> first_child_dxlnode,
		ULLONG ullPlanIdFst, ULLONG ullPlanSpaceSizeFst,
		gpos::pointer<const CDXLNode *> second_child_dxlnode,
		ULLONG ullPlanIdSnd, ULLONG ullPlanSpaceSizeSnd, BOOL fMatchPlans,
		INT iCmpSpaceSize, const CHAR *file_name);

	// run minidumps while loading metadata from passed file
	static GPOS_RESULT EresRunMinidumps(
		CMemoryPool *mp, const CHAR *rgszFileNames[], ULONG ulTests,
		ULONG *pulTestCounter, ULONG ulSessionId, ULONG ulCmdId,
		BOOL fMatchPlans, BOOL fTestSpacePruning,
		const CHAR *szMDFilePath = nullptr,
		gpos::pointer<IConstExprEvaluator *> pceeval = nullptr);

	// run all minidumps based on one metadata file
	static GPOS_RESULT EresRunMinidumpsUsingOneMDFile(
		CMemoryPool *mp, const CHAR *szMDFilePath, const CHAR *rgszFileNames[],
		ULONG *pulTestCounter, ULONG ulSessionId, ULONG ulCmdId,
		BOOL fMatchPlans, INT iCmpSpaceSize,
		gpos::pointer<IConstExprEvaluator *> pceeval = nullptr);

	// run one minidump-based test using passed MD Accessor
	static GPOS_RESULT EresRunMinidump(
		CMemoryPool *mp, CMDAccessor *md_accessor, const CHAR *rgszFileName,
		ULONG *pulTestCounter, ULONG ulSessionId, ULONG ulCmdId,
		BOOL fMatchPlans, INT iCmpSpaceSize,
		gpos::pointer<IConstExprEvaluator *> pceeval);

	// test plan sampling
	static GPOS_RESULT EresSamplePlans(const CHAR *rgszFileNames[],
									   ULONG ulTests, ULONG *pulTestCounter,
									   ULONG ulSessionId, ULONG ulCmdId);

	// check all enumerated plans using given PlanChecker function
	static GPOS_RESULT EresCheckPlans(const CHAR *rgszFileNames[],
									  ULONG ulTests, ULONG *pulTestCounter,
									  ULONG ulSessionId, ULONG ulCmdId,
									  FnPlanChecker *pfpc);

	// check the optimized plan using given DXLPlanChecker function
	// cost model must be calibrated
	// does not take ownership of pdrgpcp
	static GPOS_RESULT EresCheckOptimizedPlan(const CHAR *rgszFileNames[],
											  ULONG ulTests,
											  ULONG *pulTestCounter,
											  ULONG ulSessionId, ULONG ulCmdId,
											  FnDXLPlanChecker *pfdpc,
											  ICostModelParamsArray *pdrgpcp);

	// generate cost model used in tests
	static gpos::owner<ICostModel *>
	GetCostModel(CMemoryPool *mp)
	{
		return GPOS_NEW(mp) CCostModelGPDB(mp, GPOPT_TEST_SEGMENTS);
	}

	// create a datum with a given type, encoded value and int value
	static gpos::owner<IDatum *> CreateGenericDatum(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::owner<IMDId *> mdid_type, CWStringDynamic *pstrEncodedValue,
		LINT value);

	// Create a datum with a given type and double value
	static gpos::owner<IDatum *> CreateDoubleDatum(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::owner<IMDId *> mdid_type, CDouble value);
	// create an interval for generic data types
	// does not take ownership of mdid_type
	static gpos::owner<CConstraintInterval *> PciGenericInterval(
		CMemoryPool *mp, CMDAccessor *md_accessor, const CMDIdGPDB &mdidType,
		CColRef *colref, CWStringDynamic *pstrLower, LINT lLower,
		CRange::ERangeInclusion eriLeft, CWStringDynamic *pstrUpper,
		LINT lUpper, CRange::ERangeInclusion eriRight);

	// helper for generating a scalar compare ident to constant
	static gpos::owner<CExpression *> PexprScalarCmpIdentToConstant(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// helper for generating an exists subquery
	static gpos::owner<CExpression *> PexprExistsSubquery(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuter);

	// helper for generating a not exists subquery
	static gpos::owner<CExpression *> PexprNotExistsSubquery(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuter);

	// helper for generating an ANY subquery
	static gpos::owner<CExpression *> PexpSubqueryAny(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuter);

	// helper for generating an ALL subquery
	static gpos::owner<CExpression *> PexpSubqueryAll(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuter);

	// recursively traverses the subtree rooted at the given expression, and return
	// the first subexpression it encounters that has the given id
	static gpos::pointer<const CExpression *> PexprFirst(
		gpos::pointer<const CExpression *> pexpr,
		const COperator::EOperatorId op_id);

	// generate a scalar expression comparing scalar identifier to a constant
	static gpos::owner<CExpression *> PexprScIdentCmpConst(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprRelational,
		IMDType::ECmpType cmp_type, ULONG ulVal);

	// generate a scalar expression comparing scalar identifiers
	static gpos::owner<CExpression *> PexprScIdentCmpScIdent(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprLeft,
		gpos::pointer<CExpression *> pexprRight, IMDType::ECmpType cmp_type);

	// generate a scalar AND expression
	static gpos::owner<CExpression *> PexprAnd(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprFst,
		gpos::owner<CExpression *> pexprSnd);

	// generate a scalar OR expression
	static gpos::owner<CExpression *> PexprOr(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprFst,
		gpos::owner<CExpression *> pexprSnd);

	// default file-based metadata provider
	static gpos::owner<CMDProviderMemory *> m_pmdpf;

	// default source system id
	static CSystemId m_sysidDefault;

	// metadata file
	static const CHAR *m_szMDFileName;

	// xsd file
	static const CHAR *m_szXSDPath;

	// run Minidump-based tests in the given array of files
	static GPOS_RESULT EresUnittest_RunTests(const CHAR **rgszFileNames,
											 ULONG *pulTestCounter,
											 ULONG ulTests);

	static GPOS_RESULT EresUnittest_RunTestsWithoutAdditionalTraceFlags(
		const CHAR **rgszFileNames, ULONG *pulTestCounter, ULONG ulTests,
		BOOL fMatchPlans, BOOL fTestSpacePruning);

	// return the number of segments, default return GPOPT_TEST_SEGMENTS
	static ULONG UlSegments(gpos::pointer<COptimizerConfig *> optimizer_config);

	// create Equivalence Class based on the breakpoints
	static gpos::owner<CColRefSetArray *> createEquivalenceClasses(
		CMemoryPool *mp, gpos::pointer<CColRefSet *> pcrs,
		const INT setBoundary[]);
};	// class CTestUtils


//---------------------------------------------------------------------------
//		CTestUtils::PexprJoinPartitionedOuter
//
//	@doc:
//		Generate a randomized join expression over a partitioned table
//
//---------------------------------------------------------------------------
template <class T>
gpos::owner<CExpression *>
CTestUtils::PexprJoinPartitionedOuter(CMemoryPool *mp)
{
	return PexprJoinPartitioned<T>(mp, true /*fOuterPartitioned*/);
}

//---------------------------------------------------------------------------
//		CTestUtils::PexprJoinPartitionedInner
//
//	@doc:
//		Generate a randomized join expression over a partitioned table
//
//---------------------------------------------------------------------------
template <class T>
gpos::owner<CExpression *>
CTestUtils::PexprJoinPartitionedInner(CMemoryPool *mp)
{
	return PexprJoinPartitioned<T>(mp, false /*fOuterPartitioned*/);
}

//---------------------------------------------------------------------------
//		CTestUtils::PexprJoinPartitioned
//
//	@doc:
//		Generate a randomized join expression over a partitioned table
//
//---------------------------------------------------------------------------
template <class T>
gpos::owner<CExpression *>
CTestUtils::PexprJoinPartitioned(CMemoryPool *mp, BOOL fOuterPartitioned)
{
	// create an outer Get expression and extract a random column from it
	gpos::owner<CExpression *> pexprGet = PexprLogicalGet(mp);
	CColRef *colref = pexprGet->DeriveOutputColumns()->PcrAny();

	// create an inner partitioned Get expression
	gpos::owner<CExpression *> pexprGetPartitioned =
		PexprLogicalGetPartitioned(mp);

	// extract first partition key
	gpos::pointer<CLogicalGet *> popGetPartitioned =
		gpos::dyn_cast<CLogicalGet>(pexprGetPartitioned->Pop());
	gpos::pointer<const CColRef2dArray *> pdrgpdrgpcr =
		popGetPartitioned->PdrgpdrgpcrPartColumns();

	GPOS_ASSERT(pdrgpdrgpcr != nullptr);
	GPOS_ASSERT(0 < pdrgpdrgpcr->Size());
	gpos::pointer<CColRefArray *> colref_array = (*pdrgpdrgpcr)[0];
	GPOS_ASSERT(1 == colref_array->Size());
	CColRef *pcrPartKey = (*colref_array)[0];

	// construct a comparison pk = a
	gpos::owner<CExpression *> pexprScalar =
		CUtils::PexprScalarEqCmp(mp, CUtils::PexprScalarIdent(mp, colref),
								 CUtils::PexprScalarIdent(mp, pcrPartKey));

	if (fOuterPartitioned)
	{
		return CUtils::PexprLogicalJoin<T>(mp, std::move(pexprGetPartitioned),
										   std::move(pexprGet),
										   std::move(pexprScalar));
	}

	return CUtils::PexprLogicalJoin<T>(mp, std::move(pexprGet),
									   std::move(pexprGetPartitioned),
									   std::move(pexprScalar));
}

//---------------------------------------------------------------------------
//	@function:
//		CTestUtils::PexprLogicalJoin
//
//	@doc:
//		Generate a join expression with a random equality predicate
//
//---------------------------------------------------------------------------
template <class T>
gpos::owner<CExpression *>
CTestUtils::PexprLogicalJoin(CMemoryPool *mp,
							 gpos::owner<CExpression *> pexprLeft,
							 gpos::owner<CExpression *> pexprRight)
{
	GPOS_ASSERT(nullptr != pexprLeft);
	GPOS_ASSERT(nullptr != pexprRight);

	// get any two columns; one from each side
	CColRef *pcrLeft = pexprLeft->DeriveOutputColumns()->PcrAny();
	CColRef *pcrRight = pexprRight->DeriveOutputColumns()->PcrAny();
	gpos::owner<CExpression *> pexprEquality =
		CUtils::PexprScalarEqCmp(mp, pcrLeft, pcrRight);

	return CUtils::PexprLogicalJoin<T>(mp, std::move(pexprLeft),
									   std::move(pexprRight),
									   std::move(pexprEquality));
}

//---------------------------------------------------------------------------
//	@function:
//		CTestUtils::PexprLogicalJoin
//
//	@doc:
//		Generate randomized basic join expression
//
//---------------------------------------------------------------------------
template <class T>
gpos::owner<CExpression *>
CTestUtils::PexprLogicalJoin(CMemoryPool *mp)
{
	gpos::owner<CExpression *> pexprLeft = PexprLogicalGet(mp);
	gpos::owner<CExpression *> pexprRight = PexprLogicalGet(mp);
	// reversing the order here is a workaround for non-determinism
	// in CExpressionPreprocessorTest where join order was flipped
	// depending on the compiler used
	return PexprLogicalJoin<T>(mp, std::move(pexprRight), std::move(pexprLeft));
}

//---------------------------------------------------------------------------
//	@function:
//		CTestUtils::PexprLogicalApply
//
//	@doc:
//		Generate top-level apply
//
//---------------------------------------------------------------------------
template <class T>
gpos::owner<CExpression *>
CTestUtils::PexprLogicalApply(CMemoryPool *mp)
{
	gpos::owner<CExpression *> pexprOuter = PexprLogicalGet(mp);
	CColRefSet *outer_refs = pexprOuter->DeriveOutputColumns();

	gpos::owner<CExpression *> pexprInner =
		PexprLogicalSelectCorrelated(mp, outer_refs, 3);
	gpos::owner<CExpression *> pexprPredicate =
		CUtils::PexprScalarConstBool(mp, true /*value*/);

	gpos::owner<COperator *> pop = GPOS_NEW(mp) T(mp);
	return GPOS_NEW(mp)
		CExpression(mp, std::move(pop), std::move(pexprOuter),
					std::move(pexprInner), std::move(pexprPredicate));
}

//---------------------------------------------------------------------------
//	@function:
//		CTestUtils::PexprLogicalApplyWithOuterRef
//
//	@doc:
//		Generate top-level apply
//
//---------------------------------------------------------------------------
template <class T>
gpos::owner<CExpression *>
CTestUtils::PexprLogicalApplyWithOuterRef(CMemoryPool *mp)
{
	gpos::owner<CExpression *> pexprOuter = PexprLogicalGet(mp);
	CColRefSet *outer_refs = pexprOuter->DeriveOutputColumns();

	gpos::owner<CExpression *> pexprInner =
		PexprLogicalSelectCorrelated(mp, outer_refs, 3);
	gpos::owner<CExpression *> pexprPredicate =
		CUtils::PexprScalarConstBool(mp, true /*value*/);

	gpos::pointer<CColRefSet *> pcrsOuterRef =
		pexprInner->DeriveOuterReferences();
	GPOS_ASSERT(1 == pcrsOuterRef->Size());
	CColRef *colref = pcrsOuterRef->PcrFirst();

	gpos::owner<CColRefArray *> colref_array = GPOS_NEW(mp) CColRefArray(mp);
	colref_array->Append(colref);
	gpos::owner<COperator *> pop = GPOS_NEW(mp)
		T(mp, std::move(colref_array), COperator::EopScalarSubquery);
	return GPOS_NEW(mp)
		CExpression(mp, std::move(pop), std::move(pexprOuter),
					std::move(pexprInner), std::move(pexprPredicate));
}

}  // namespace gpopt


#endif	// !GPOPT_CTestUtils_H

// EOF
