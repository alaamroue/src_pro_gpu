/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */


#include "common.h"
#include "CDomainRemote.h"

/*
 *  Constructor
 */
CDomainRemote::CDomainRemote(void)
{
	// Populate summary details to avoid errors
	pSummary.bAuthoritative = false;
	pSummary.dResolutionX = 0.0;
	pSummary.dResolutionY = 0.0;
	pSummary.ucFloatPrecision = 0;
	pSummary.uiNodeID = 0;
	pSummary.uiDomainID = 0;
	pSummary.uiLocalDeviceID = 0;
	pSummary.ulColCount = 0;
	pSummary.ulRowCount = 0;
}

/*
 *  Destructor
 */
CDomainRemote::~CDomainRemote(void)
{
	// ...
}

/*
 *	Fetch summary information for this domain
 */
CDomainBase::DomainSummary CDomainRemote::getSummary()
{
	return this->pSummary;
}

