//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CSerializableQuery.h
//
//	@doc:
//		Serializable query object used to dump the query when an exception is raised;
//---------------------------------------------------------------------------
#ifndef GPOS_CSerializableQuery_H
#define GPOS_CSerializableQuery_H

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/error/CSerializable.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLNode.h"

using namespace gpos;
using namespace gpdxl;

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CSerializableQuery
//
//	@doc:
//		Serializable query object
//
//---------------------------------------------------------------------------
class CSerializableQuery : public CSerializable
{
private:
	CMemoryPool *m_mp;

	// query DXL node;
	gpos::pointer<const CDXLNode *> m_query_dxl_root;

	// query output
	gpos::pointer<const CDXLNodeArray *> m_query_output;

	// CTE DXL nodes
	gpos::pointer<const CDXLNodeArray *> m_cte_producers;


public:
	CSerializableQuery(const CSerializableQuery &) = delete;

	// ctor
	CSerializableQuery(
		CMemoryPool *mp, gpos::pointer<const CDXLNode *> query,
		gpos::pointer<const CDXLNodeArray *> query_output_dxlnode_array,
		gpos::pointer<const CDXLNodeArray *> cte_producers);

	// dtor
	~CSerializableQuery() override;

	// serialize object to passed stream
	void Serialize(COstream &oos) override;

};	// class CSerializableQuery
}  // namespace gpopt

#endif	// !GPOS_CSerializableQuery_H

// EOF
