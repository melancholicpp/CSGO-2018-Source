//============ Copyright (c) Valve Corporation, All rights reserved. ============
//
//
//
//===============================================================================

#ifndef SOS_OP_ENTITY_INFO_H
#define SOS_OP_ENTITY_INFO_H
#ifdef _WIN32
#pragma once
#endif

#include "sos_op.h"

//-----------------------------------------------------------------------------
// simple operator for setting a single position
//-----------------------------------------------------------------------------
struct CSosOperatorEntityInfo_t : CSosOperator_t
{

	SOS_OUTPUT_FLOAT( m_flOutPosition, SO_VEC3 )
	SOS_OUTPUT_FLOAT( m_flOutPosition_x, SO_SINGLE )
	SOS_OUTPUT_FLOAT( m_flOutPosition_y, SO_SINGLE )
	SOS_OUTPUT_FLOAT( m_flOutPosition_z, SO_SINGLE )

	SOS_OUTPUT_FLOAT( m_flOutAngles, SO_VEC3 )

	SOS_OUTPUT_FLOAT( m_flOutVelocity, SO_SINGLE )
	SOS_OUTPUT_FLOAT( m_flOutVelocityVector, SO_VEC3 )

	SOS_OUTPUT_FLOAT( m_flOutVelocityVector_x, SO_SINGLE )
	SOS_OUTPUT_FLOAT( m_flOutVelocityVector_y, SO_SINGLE )
	SOS_OUTPUT_FLOAT( m_flOutVelocityVector_z, SO_SINGLE )
	SOS_OUTPUT_FLOAT( m_flOutVelocityXY, SO_SINGLE )

	SOS_INPUT_FLOAT( m_flInputEntityIndex, SO_SINGLE )

	float m_flPrevTime;
	Vector m_vecPrevPosition;

};

class CSosOperatorEntityInfo : public CSosOperator
{
	SOS_HEADER_DESC( CSosOperatorEntityInfo )
};

#endif // SOS_OP_ENTITY_INFO_H