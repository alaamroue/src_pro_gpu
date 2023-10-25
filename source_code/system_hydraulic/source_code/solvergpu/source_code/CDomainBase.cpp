/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */


#include "common.h"
#include "CDomainBase.h"
#include "CDomainCartesian.h"
#include "CDomainLink.h"


//Constructor
CDomainBase::CDomainBase(void)
{
	this->bPrepared			= false;
	this->uiRollbackLimit	= 999999999;

	pDataProgress.dBatchTimesteps = 0;
	pDataProgress.dCurrentTime = 0.0;
	pDataProgress.dCurrentTimestep = 0.0;
	pDataProgress.uiBatchSize = 0;
	pDataProgress.uiBatchSkipped = 0;
	pDataProgress.uiBatchSuccessful = 0;
}

//Destructor
CDomainBase::~CDomainBase(void)
{
	for (unsigned int uiID = 0; uiID < links.size(); ++uiID)
		delete links[uiID];

	model::log->logInfo("The domain base has been released.");
}

//Helper function that create a new class of the appropriate type for the domain we have.Create a domain of the specified type 
CDomainBase* CDomainBase::createDomain(unsigned char cType)
{
	// Cartesian grid?
	if ( cType == model::domainStructureTypes::kStructureCartesian )
	{
		return static_cast<CDomainBase*>(new CDomainCartesian); //deleted by CModel
	}

	model::doError(
		"Unrecognized domain data store type identifier passed for creation",
		model::errorCodes::kLevelFatal,
		"CDomainBase* CDomainBase::createDomain(unsigned char cType)",
		"Please contact the developers"
	);
	return NULL;
}

//Is this domain ready to be used for a model run?
bool	CDomainBase::isInitialised()
{
	return true;
}

//Return the total number of cells in the domain
unsigned long	CDomainBase::getCellCount()
{
	return this->ulCellCount;
}

//Fetch summary information for this domain
CDomainBase::DomainSummary CDomainBase::getSummary()
{
	CDomainBase::DomainSummary pSummary;

	pSummary.uiNodeID = 0;
	
	return pSummary;
}

//Add a new link to another domain
void CDomainBase::addLink(CDomainLink* pLink)
{
	links.push_back(pLink);
}

//Add a new link which is dependent on this domain
void CDomainBase::addDependentLink(CDomainLink* pLink)
{
	dependentLinks.push_back(pLink);
}

//Fetch a link with a specific domain
CDomainLink* CDomainBase::getLinkFrom( unsigned int uiSourceDomainID )
{
	for (unsigned int i = 0; i < links.size(); i++)
	{
		if ( links[i]->getSourceDomainID() == uiSourceDomainID )
			return links[i];
	}
	
	return NULL;
}

//Fetch a cell ID based on Cartesian assumption and data held in the summary
unsigned long	CDomainBase::getCellID(unsigned long ulX, unsigned long ulY)
{
	DomainSummary pSummary = this->getSummary();
	return (ulY * pSummary.ulColCount) + ulX;
}

//Fetch the X and Y indices for a cell using its ID
void	CDomainBase::getCellIndices(unsigned long ulID, unsigned long* lIdxX, unsigned long* lIdxY)
{
	*lIdxX = ulID % this->getSummary().ulColCount;
	*lIdxY = (ulID - *lIdxX) / this->getSummary().ulColCount;
}

//Fetch the ID for a neighboring cell in the domain
unsigned long	CDomainBase::getNeighbourID(unsigned long ulCellID, unsigned char ucDirection)
{
	unsigned long lIdxX = 0;
	unsigned long lIdxY = 0;
	getCellIndices(ulCellID, &lIdxX, &lIdxY);

	switch (ucDirection)
	{
	case direction::north:
		++lIdxY;
		break;
	case direction::east:
		++lIdxX;
		break;
	case direction::south:
		--lIdxY;
		break;
	case direction::west:
		--lIdxX;
		break;
	}

	return getCellID(lIdxX, lIdxY);
}

//Identify a suitable rollback limit automatically
void CDomainBase::setRollbackLimit()
{
	unsigned int uiLimit = 999999999;

	for (unsigned int i = 0; i < links.size(); i++)
	{
		if (uiLimit >= 999999999 || links[i]->getSmallestOverlap() - 1 < uiLimit)
			uiLimit = links[i]->getSmallestOverlap() - 1;
	}

	uiRollbackLimit = uiLimit;
}

//When a domain is told to rollback, any link state data becomes invalid
void CDomainBase::markLinkStatesInvalid()
{
	for (unsigned int i = 0; i < links.size(); i++)
	{
		links[i]->markInvalid();
	}
}

//Are all our links at the specified time?
bool CDomainBase::isLinkSetAtTime( double dCheckTime )
{
	for (unsigned int i = 0; i < links.size(); i++)
	{
		if ( !links[i]->isAtTime( dCheckTime ) )
			return false;
	}
	
	return true;
}

//Send our link data to other nodes
bool CDomainBase::sendLinkData()
{
	bool bAlreadySent = true;

	for (unsigned int i = 0; i < links.size(); i++)
	{
		if ( !links[i]->sendOverMPI() )
			bAlreadySent = false;
	}
	
	return bAlreadySent;
}
