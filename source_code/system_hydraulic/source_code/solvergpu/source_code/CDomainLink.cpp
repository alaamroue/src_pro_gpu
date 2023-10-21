/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

#include <math.h>
#include <cmath>
#include <stdlib.h>

#include "common.h"
#include "CDomainLink.h"
#include "CDomainCartesian.h"	// TEMP: Remove me!
#include "CDomainManager.h"				// TEMP: Remove me!

using std::min;
using std::max;

/*
 *  Constructor
 */
CDomainLink::CDomainLink( CDomainBase* pTarget, CDomainBase *pSource )
{
	this->uiTargetDomainID = pTarget->getID();
	this->uiSourceDomainID = pSource->getID();

	this->iTargetNodeID = -1;
	
	// Start as invalid
	this->dValidityTime = -1.0;
	this->bSent 		= true;
	this->uiSmallestOverlap = 999999999;

	model::log->writeLine("Generating link definitions between domains #" + toStringExact(this->uiTargetDomainID + 1) 
		+ " and #" + toStringExact(this->uiSourceDomainID + 1));

	this->generateDefinitions( pTarget, pSource );
}

/*
 *  Destructor
 */
CDomainLink::~CDomainLink(void)
{
	for (unsigned int i = 0; i < linkDefs.size(); i++)
	{
		delete[] linkDefs[i].vStateData;
	}
}

/*
 *	Return whether or not it is possible for two domains to be linked
 *	together in a CDomainLink class.
 */
bool CDomainLink::canLink(CDomainBase* pA, CDomainBase* pB)
{
	CDomainBase::DomainSummary pSumA = pA->getSummary();
	CDomainBase::DomainSummary pSumB = pB->getSummary();
	return false;
}

/*
 *	Import data received through MPI
 */
void	CDomainLink::pullFromMPI(double dCurrentTime, char* pData)
{
#ifdef DEBUG_MPI
	model::log->writeLine("[DEBUG] Importing link data via MPI... Data time: " + Util::secondsToTime(dCurrentTime) + ", Current time: " + Util::secondsToTime(this->dValidityTime));
#endif

	if ( this->dValidityTime >= dCurrentTime )
		return;

	unsigned int uiOffset = 0;
		
	for( unsigned int i = 0; i < this->linkDefs.size(); i++ )
	{
		memcpy(
			this->linkDefs[i].vStateData,
			&pData[ uiOffset ],
			this->linkDefs[i].ulSize
		);
		uiOffset += this->linkDefs[i].ulSize;
	}
		
	this->dValidityTime = dCurrentTime;
}

/*
 *	Download data for the link from a memory buffer
 */
void	CDomainLink::pullFromBuffer(double dCurrentTime, COCLBuffer* pBuffer)
{
	if ( this->dValidityTime >= dCurrentTime &&
		 this->bSent )
		return;

	if ( this->dValidityTime < dCurrentTime )
	{
	
		for (unsigned int i = 0; i < this->linkDefs.size(); i++)
		{
#ifdef DEBUG_MPI
				model::log->writeLine("[DEBUG] Should now be downloading data from buffer at time " + Util::secondsToTime(dCurrentTime));
#endif
				pBuffer->queueReadPartial(
					this->linkDefs[i].ulOffsetSource,
					this->linkDefs[i].ulSize,
					this->linkDefs[i].vStateData
				);
		}
		
		this->dValidityTime = dCurrentTime;
		this->bSent = false;
		
	} else {
#ifdef DEBUG_MPI
		model::log->writeLine("[DEBUG] Not downloading data at " + Util::secondsToTime(dCurrentTime) + " as validity time is " + Util::secondsToTime(dValidityTime));
#endif
	}
}

/*
 *	Send the data over MPI if it hasn't already been sent...
 */
bool	CDomainLink::sendOverMPI()
{
	if ( this->bSent )
		return true;

	this->bSent = true;
	
	return false;
}

/*
 *	Push data to a memory buffer
 */
void	CDomainLink::pushToBuffer(COCLBuffer* pBuffer)
{
	// Once there is a check in the sync ready code for the current timestep of each link
	// this should become redundant...
	// TODO: Remove this later...
	if (this->dValidityTime < 0.0) return;

	for (unsigned int i = 0; i < this->linkDefs.size(); i++)
	{
#ifdef DEBUG_MPI
		model::log->writeLine("[DEBUG] Should now be pushing data to buffer at time " + Util::secondsToTime(dValidityTime) + " (" + toStringExact(this->linkDefs[i].ulSize) + " bytes)");
#endif
		pBuffer->queueWritePartial(
			this->linkDefs[i].ulOffsetTarget,
			this->linkDefs[i].ulSize,
			this->linkDefs[i].vStateData
		);
	}
}

/*
 *	Is this link at the specified time yet?
 */
bool	CDomainLink::isAtTime( double dCheckTime )
{
	if ( fabs( this->dValidityTime - dCheckTime ) > 1E-5 )
		return false;
		
	return true;
}

/*
 *	Identify all the areas of contiguous memory which overlap so we know what needs exchanging
 */
void	CDomainLink::generateDefinitions(CDomainBase* pTarget, CDomainBase *pSource)
{

}
