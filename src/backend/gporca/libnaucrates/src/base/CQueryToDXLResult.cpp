//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CQueryToDXLResult.cpp
//
//	@doc:
//		Implementation of the methods for accessing the result of the translation
//---------------------------------------------------------------------------


#include "naucrates/base/CQueryToDXLResult.h"

#include "gpos/common/owner.h"


using namespace gpdxl;
using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CQueryToDXLResult::CQueryToDXLResult
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CQueryToDXLResult::CQueryToDXLResult(gpos::Ref<CDXLNode> query,
									 gpos::Ref<CDXLNodeArray> query_output,
									 gpos::Ref<CDXLNodeArray> cte_producers)
	: m_query_dxl(std::move(query)),
	  m_query_output(std::move(query_output)),
	  m_cte_producers(std::move(cte_producers))
{
	GPOS_ASSERT(nullptr != m_query_dxl);
}

//---------------------------------------------------------------------------
//	@function:
//		CQueryToDXLResult::~CQueryToDXLResult
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CQueryToDXLResult::~CQueryToDXLResult()
{
	;
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CQueryToDXLResult::GetOutputColumnsDXLArray
//
//	@doc:
//		Return the array of dxl nodes representing the query output
//
//---------------------------------------------------------------------------
const CDXLNodeArray *
CQueryToDXLResult::GetOutputColumnsDXLArray() const
{
	return m_query_output.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CQueryToDXLResult::GetCTEProducerDXLArray
//
//	@doc:
//		Return the array of CTEs
//
//---------------------------------------------------------------------------
const CDXLNodeArray *
CQueryToDXLResult::GetCTEProducerDXLArray() const
{
	return m_cte_producers.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CQueryToDXLResult::CreateDXLNode
//
//	@doc:
//		Return the DXL node representing the query
//
//---------------------------------------------------------------------------
const CDXLNode *
CQueryToDXLResult::CreateDXLNode() const
{
	return m_query_dxl.get();
}



// EOF
