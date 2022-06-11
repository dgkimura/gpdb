//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDScalarOpGPDB.h
//
//	@doc:
//		Class for representing GPDB-specific scalar operators in the MD cache
//---------------------------------------------------------------------------



#ifndef GPMD_CMDScalarOpGPDB_H
#define GPMD_CMDScalarOpGPDB_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/md/IMDScalarOp.h"

namespace gpmd
{
using namespace gpos;


//---------------------------------------------------------------------------
//	@class:
//		CMDScalarOpGPDB
//
//	@doc:
//		Class for representing GPDB-specific scalar operators in the MD cache
//
//---------------------------------------------------------------------------
class CMDScalarOpGPDB : public IMDScalarOp
{
private:
	// memory pool
	CMemoryPool *m_mp;

	// DXL for object
	const CWStringDynamic *m_dxl_str;

	// operator id
	gpos::owner<IMDId *> m_mdid;

	// operator name
	CMDName *m_mdname;

	// type of left operand
	gpos::owner<IMDId *> m_mdid_type_left;

	// type of right operand
	gpos::owner<IMDId *> m_mdid_type_right;

	// type of result operand
	gpos::owner<IMDId *> m_mdid_type_result;

	// id of function which implements the operator
	gpos::owner<IMDId *> m_func_mdid;

	// id of commute operator
	gpos::owner<IMDId *> m_mdid_commute_opr;

	// id of inverse operator
	gpos::owner<IMDId *> m_mdid_inverse_opr;

	// comparison type for comparison operators
	IMDType::ECmpType m_comparision_type;

	// does operator return NULL when all inputs are NULL?
	BOOL m_returns_null_on_null_input;

	// operator classes this operator belongs to
	gpos::owner<IMdIdArray *> m_mdid_opfamilies_array;

	// compatible hash op family
	gpos::owner<IMDId *> m_mdid_hash_opfamily;

	// compatible legacy hash op family using legacy (cdbhash) opclass
	gpos::owner<IMDId *> m_mdid_legacy_hash_opfamily;

	// does operator preserve the NDV of its input(s)
	// (used for cardinality estimation)
	BOOL m_is_ndv_preserving;

public:
	CMDScalarOpGPDB(const CMDScalarOpGPDB &) = delete;

	// ctor/dtor
	CMDScalarOpGPDB(CMemoryPool *mp, gpos::owner<IMDId *> mdid, CMDName *mdname,
					gpos::owner<IMDId *> mdid_type_left,
					gpos::owner<IMDId *> mdid_type_right,
					gpos::owner<IMDId *> result_type_mdid,
					gpos::owner<IMDId *> mdid_func,
					gpos::owner<IMDId *> mdid_commute_opr,
					gpos::owner<IMDId *> m_mdid_inverse_opr,
					IMDType::ECmpType cmp_type, BOOL returns_null_on_null_input,
					gpos::owner<IMdIdArray *> mdid_opfamilies_array,
					gpos::owner<IMDId *> m_mdid_hash_opfamily,
					gpos::owner<IMDId *> mdid_legacy_hash_opfamily,
					BOOL is_ndv_preserving);

	~CMDScalarOpGPDB() override;

	// accessors
	const CWStringDynamic *
	GetStrRepr() const override
	{
		return m_dxl_str;
	}

	// operator id
	gpos::pointer<IMDId *> MDId() const override;

	// operator name
	CMDName Mdname() const override;

	// left operand type id
	gpos::pointer<IMDId *> GetLeftMdid() const override;

	// right operand type id
	gpos::pointer<IMDId *> GetRightMdid() const override;

	// resulttype id
	gpos::pointer<IMDId *> GetResultTypeMdid() const override;

	// implementer function id
	gpos::pointer<IMDId *> FuncMdId() const override;

	// commutor id
	gpos::pointer<IMDId *> GetCommuteOpMdid() const override;

	// inverse operator id
	gpos::pointer<IMDId *> GetInverseOpMdid() const override;

	// is this an equality operator
	BOOL IsEqualityOp() const override;

	// does operator return NULL when all inputs are NULL?
	// STRICT implies NULL-returning, but the opposite is not always true,
	// the implementation in GPDB returns what STRICT property states
	BOOL ReturnsNullOnNullInput() const override;

	// preserves NDVs of its inputs?
	BOOL IsNDVPreserving() const override;

	// comparison type
	IMDType::ECmpType ParseCmpType() const override;

	// serialize object in DXL format
	void Serialize(gpdxl::CXMLSerializer *xml_serializer) const override;

	// number of classes this operator belongs to
	ULONG OpfamiliesCount() const override;

	// operator class at given position
	gpos::pointer<IMDId *> OpfamilyMdidAt(ULONG pos) const override;

	// compatible hash opfamily
	gpos::pointer<IMDId *> HashOpfamilyMdid() const override;

#ifdef GPOS_DEBUG
	// debug print of the type in the provided stream
	void DebugPrint(IOstream &os) const override;
#endif
};
}  // namespace gpmd

#endif	// !GPMD_CMDScalarOpGPDB_H

// EOF
