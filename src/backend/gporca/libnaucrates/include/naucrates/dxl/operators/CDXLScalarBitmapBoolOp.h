//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLScalarBitmapBoolOp.h
//
//	@doc:
//		Class for representing DXL bitmap boolean operator
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarBitmapBoolOp_H
#define GPDXL_CDXLScalarBitmapBoolOp_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"


namespace gpdxl
{
	using namespace gpos;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarBitmapBoolOp
	//
	//	@doc:
	//		Class for representing DXL bitmap boolean operator
	//
	//---------------------------------------------------------------------------
	class CDXLScalarBitmapBoolOp : public CDXLOperator
	{

		public:
		
			// type of bitmap operator
			enum EdxlBitmapBoolOp
			{
				EdxlbitmapAnd,
				EdxlbitmapOr,
				EdxlbitmapSentinel
			};
		
		private:

			// operator type
			const EdxlBitmapBoolOp m_bitmap_op_type;

			// private copy ctor
			CDXLScalarBitmapBoolOp(const CDXLScalarBitmapBoolOp&);

		public:
			// ctor
			CDXLScalarBitmapBoolOp(CMemoryPool *mp, EdxlBitmapBoolOp bitmap_op_type);
			
			// dtor 
			virtual
			~CDXLScalarBitmapBoolOp();

			// dxl operator type
			virtual
			Edxlopid GetDXLOperator() const;

			// bitmap operator type
			EdxlBitmapBoolOp GetDXLBitmapOpType() const;

			// name of the DXL operator name
			virtual
			const CWStringConst *GetOpNameStr() const;

			virtual
			Edxloptype
			GetDXLOperatorType() const
			{
				return EdxloptypeScalar;
			}
			
			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *dxlnode) const;


#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			virtual
			void AssertValid(const CDXLNode *dxlnode, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// conversion function
			static
			CDXLScalarBitmapBoolOp *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarBitmapBoolOp == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLScalarBitmapBoolOp*>(dxl_op);
			}
	};
}

#endif // !GPDXL_CDXLScalarBitmapBoolOp_H

// EOF
