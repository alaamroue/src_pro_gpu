/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

#include <limits>
#include <stdio.h>
#include <cstring>

#include "common.h"
#include "CModel.h"
#include "CDomainManager.h"
#include "CDomain.h"
#include "CScheme.h"

#include "CExecutorControlOpenCL.h"
#include "CDomainCartesian.h"

/*
 *  Constructor
 */
CDomainCartesian::CDomainCartesian(void)
{
	// Default values will trigger errors on validation
	this->dCellResolutionX			= std::numeric_limits<double>::quiet_NaN();
	this->dCellResolutionY			= std::numeric_limits<double>::quiet_NaN();
	this->ulRows					= std::numeric_limits<unsigned long>::quiet_NaN();
	this->ulCols					= std::numeric_limits<unsigned long>::quiet_NaN();
	this->bUseOptimizedBoundary		= false;
	this->ulCouplingArraySize		= 0;
}

/*
 *  Destructor
 */
CDomainCartesian::~CDomainCartesian(void)
{
	// ...
}


/*
 *  Does the domain contain all of the required data yet?
 */
bool	CDomainCartesian::validateDomain( bool bQuiet )
{
	// Got a resolution?
	if ( this->dCellResolutionX == NAN )
	{
		if ( !bQuiet ) model::doError(
			"Domain cell resolution not defined",
			model::errorCodes::kLevelWarning
		);
		return false;
	}
	if (this->dCellResolutionY == NAN)
	{
		if (!bQuiet) model::doError(
			"Domain cell resolution not defined",
			model::errorCodes::kLevelWarning
		);
		return false;
	}

	if (this->ulRows == NAN || this->ulCols == NAN)
	{
		if (!bQuiet) model::doError(
			"Rows/Cols have not been defined",
			model::errorCodes::kLevelWarning
		);
		return false;
	}

	return true;
}

/*
 *  Create the required data structures etc.
 */
void	CDomainCartesian::prepareDomain()
{
	if ( !this->validateDomain( true ) ) 
	{
		model::doError(
			"Cannot prepare the domain. Invalid specification.",
			model::errorCodes::kLevelModelStop
		);
		return;
	}

	this->bPrepared = true;
	this->logDetails();
}

/*
 *  Write useful information about this domain to the log file
 */
void	CDomainCartesian::logDetails()
{
	model::log->writeDivide();
	unsigned short	wColour			= model::cli::colourInfoBlock;

	model::log->writeLine( "REGULAR CARTESIAN GRID DOMAIN", true, wColour );
	model::log->writeLine( "  Device number:     " + toStringExact( this->pDevice->uiDeviceNo ), true, wColour );
	model::log->writeLine( "  Cell count:        " + toStringExact( this->ulCellCount ), true, wColour );
	model::log->writeLine( "  Cell resolution:   " + toStringExact( this->dCellResolutionX ), true, wColour );
	model::log->writeLine( "  Cell dimensions:   [" + toStringExact( this->ulCols ) + ", " + 
														 toStringExact( this->ulRows ) + "]", true, wColour );

	model::log->writeDivide();
}

/*
 *  Set cell resolution
 */
void	CDomainCartesian::setCellResolution( double dResolutionX, double dResolutionY)
{
	this->dCellResolutionX = dResolutionX;
	this->dCellResolutionY = dResolutionY;
	this->updateCellStatistics();
}

/*
 *   Fetch cell resolution
 */
void	CDomainCartesian::getCellResolution( double* dResolutionX, double* dResolutionY)
{
	*dResolutionX = this->dCellResolutionX;
	*dResolutionY = this->dCellResolutionY;
}

/*
 *  Update basic statistics on the number of cells etc.
 */
void	CDomainCartesian::updateCellStatistics()
{
	// Do we have enough information to proceed?...

	// Got a resolution?
	if ( this->dCellResolutionX == NAN )
	{
		return;
	}
	if (this->dCellResolutionY == NAN)
	{
		return;
	}

	if (this->ulRows == NAN)
	{
		return;
	}
	if (this->ulCols == NAN)
	{
		return;
	}

	this->ulCellCount = this->ulRows * this->ulCols;
}

/*
 *  
 */
void	CDomainCartesian::setCols(unsigned long value)
{
	this->ulCols = value;
	this->updateCellStatistics();
}

/*
 *  
 */
void	CDomainCartesian::setRows(unsigned long value)
{
	this->ulRows = value;
	this->updateCellStatistics();
}

/*
 *  Return the total number of rows in the domain
 */
unsigned long	CDomainCartesian::getRows()
{
	return this->ulRows;
}

/*
 *  Return the total number of columns in the domain
 */
unsigned long	CDomainCartesian::getCols()
{
	return this->ulCols;
}

/*
 *
 */
void	CDomainCartesian::setUseOptimizedCoupling(bool state)
{
	this->bUseOptimizedBoundary = state;
}

/*
 *  
 */
bool	CDomainCartesian::getUseOptimizedCoupling()
{
	return this->bUseOptimizedBoundary;
}

/*
 *
 */
void	CDomainCartesian::setOptimizedCouplingSize(unsigned long value)
{
	this->ulCouplingArraySize = value;
}

/*
 *  
 */
unsigned long	CDomainCartesian::getOptimizedCouplingSize()
{
	return this->ulCouplingArraySize;
}
/*
 *  Get a cell ID from an X and Y index
 */
unsigned long	CDomainCartesian::getCellID( unsigned long ulX, unsigned long ulY )
{
	return ( ulY * this->getCols() ) + ulX;
}

/*
 *  Calculate the total volume present in all of the cells
 */
double	CDomainCartesian::getVolume()
{
	double dVolume = 0.0;

	for( unsigned int i = 0; i < this->ulCellCount; ++i )
	{
		if ( this->isDoublePrecision() )
		{
			dVolume += ( this->dCellStates[i].s[0] - this->dBedElevations[i] ) *
					   this->dCellResolutionX * this->dCellResolutionY;
		} else {
			dVolume += ( this->fCellStates[i].s[0] - this->fBedElevations[i] ) *
					   this->dCellResolutionX * this->dCellResolutionY;
		}
	}

	return dVolume;
}

/*
 *  Manipulate the topography to impose boundary conditions
 */
void	CDomainCartesian::imposeBoundaryModification(unsigned char ucDirection, unsigned char ucType)
{
	unsigned long ulMinX, ulMaxX, ulMinY, ulMaxY;

	if (ucDirection == edge::kEdgeE) 
		{ ulMinY = 0; ulMaxY = this->ulRows - 1; ulMinX = this->ulCols - 1; ulMaxX = this->ulCols - 1; };
	if (ucDirection == edge::kEdgeW)
		{ ulMinY = 0; ulMaxY = this->ulRows - 1; ulMinX = 0; ulMaxX = 0; };
	if (ucDirection == edge::kEdgeN)
		{ ulMinY = this->ulRows - 1; ulMaxY = this->ulRows - 1; ulMinX = 0; ulMaxX = this->ulCols - 1; };
	if (ucDirection == edge::kEdgeS)
		{ ulMinY = 0; ulMaxY = 0; ulMinX = 0; ulMaxX = this->ulCols - 1; };

	for (unsigned long x = ulMinX; x <= ulMaxX; x++)
	{
		for (unsigned long y = ulMinY; y <= ulMaxY; y++)
		{
			if (ucType == CDomainCartesian::boundaryTreatment::kBoundaryClosed)
			{
				this->setBedElevation(
					this->getCellID( x, y ),
					9999.9
				);
			}
		}
	}
}

/*
 *  Write output files to disk
 */
double*	CDomainCartesian::readBuffers_opt_h()
{
	// Read the data back first...
	// TODO: Review whether this is necessary, isn't it a sync point anyway?
	pDevice->blockUntilFinished();
	pScheme->readDomainAll();
	pDevice->blockUntilFinished();

	unsigned long	ulCellID;
	double* values = new double[this->getRows() * this->getCols()];

	//double Volume = 0;
	for (unsigned long iRow = 0; iRow < this->getRows(); ++iRow) {
		for (unsigned long iCol = 0; iCol < this->getCols(); ++iCol) {
			ulCellID = this->getCellID(iCol, iRow);
			values[ulCellID] = this->getStateValue(ulCellID, model::domainValueIndices::kValueFreeSurfaceLevel) - this->getBedElevation(ulCellID);
			//Volume += values[ulCellID];
		}
	}

	//std::cout << "" << Volume << std::endl;
	return values;
}

/*
 *  Write output files to disk
 */
double** CDomainCartesian::readBuffers_h_vx_vy()
{
	// Read the data back first...
	// TODO: Review whether this is necessary, isn't it a sync point anyway?
	pDevice->blockUntilFinished();
	pScheme->readDomainAll();
	pDevice->blockUntilFinished();

	unsigned long	ulCellID;
	double** FullList = new double* [3];
	double* opt_h = new double[this->getRows() * this->getCols()];
	double* v_x = new double[this->getRows() * this->getCols()];
	double* v_y = new double[this->getRows() * this->getCols()];

	double dDepth = 0.0;
	double dV_x = 0.0;
	double dV_y = 0.0;

	//double Volume = 0;
	for (unsigned long iRow = 0; iRow < this->getRows(); ++iRow) {
		for (unsigned long iCol = 0; iCol < this->getCols(); ++iCol) {
			ulCellID = this->getCellID(iCol, iRow);
			dDepth = this->getStateValue(ulCellID, model::domainValueIndices::kValueFreeSurfaceLevel) - this->getBedElevation(ulCellID);
			dV_x = dDepth > 1E-8 ? (this->getStateValue(ulCellID, model::domainValueIndices::kValueDischargeX) / dDepth) : 0.0;
			dV_y = dDepth > 1E-8 ? (this->getStateValue(ulCellID, model::domainValueIndices::kValueDischargeY) / dDepth) : 0.0;

			opt_h[ulCellID] = dDepth;
			v_x[ulCellID] = dV_x;
			v_y[ulCellID] = dV_y;

			//Volume += values[ulCellID];
		}
	}

	FullList[0] = opt_h;
	FullList[1] = v_x;
	FullList[2] = v_y;

	//std::cout << "" << Volume << std::endl;
	return FullList;
}

/*
 *  Read Velocity in x buffer to pointer array (Created on Heap) (Caller needs to destory this after)
 */
double* CDomainCartesian::readBuffers_v_x()
{
	// Read the data back first...
	// TODO: Review whether this is necessary, isn't it a sync point anyway?
	pDevice->blockUntilFinished();
	pScheme->readDomainAll();
	pDevice->blockUntilFinished();

	unsigned long	ulCellID;
	double dDepth, dResult;
	double* values = new double[this->getRows() * this->getCols()];

	//double Volume = 0;
	for (unsigned long iRow = 0; iRow < this->getRows(); ++iRow) {
		for (unsigned long iCol = 0; iCol < this->getCols(); ++iCol) {
			ulCellID = this->getCellID(iCol, iRow);
			dDepth = this->getStateValue(ulCellID, model::domainValueIndices::kValueFreeSurfaceLevel) - this->getBedElevation(ulCellID);

			dResult = dDepth > 1E-8 ? (this->getStateValue(ulCellID, model::domainValueIndices::kValueDischargeX) / dDepth) : 0.0;

			values[ulCellID] = dResult;
		}
	}

	return values;
}

/*
 *  Read Velocity in x buffer to pointer array (Created on Heap) (Caller needs to destory this after)
 */
double* CDomainCartesian::readBuffers_v_y()
{
	// Read the data back first...
	// TODO: Review whether this is necessary, isn't it a sync point anyway?
	pDevice->blockUntilFinished();
	pScheme->readDomainAll();
	pDevice->blockUntilFinished();

	unsigned long	ulCellID;
	double dDepth, dResult;
	double* values = new double[this->getRows() * this->getCols()];

	//double Volume = 0;
	for (unsigned long iRow = 0; iRow < this->getRows(); ++iRow) {
		for (unsigned long iCol = 0; iCol < this->getCols(); ++iCol) {
			ulCellID = this->getCellID(iCol, iRow);
			dDepth = this->getStateValue(ulCellID, model::domainValueIndices::kValueFreeSurfaceLevel) - this->getBedElevation(ulCellID);

			dResult = dDepth > 1E-8 ? (this->getStateValue(ulCellID, model::domainValueIndices::kValueDischargeY) / dDepth) : 0.0;

			values[ulCellID] = dResult;
		}
	}

	return values;
}


/*
 *	Fetch summary information for this domain
 */
CDomainBase::DomainSummary CDomainCartesian::getSummary()
{
	CDomainBase::DomainSummary pSummary;
	
	pSummary.bAuthoritative = true;

	pSummary.uiDomainID		= this->uiID;
	pSummary.uiNodeID = 0;
	pSummary.uiLocalDeviceID = this->getDevice()->getDeviceID();
	pSummary.ulColCount		= this->ulCols;
	pSummary.ulRowCount		= this->ulRows;
	pSummary.ucFloatPrecision = ( this->isDoublePrecision() ? model::floatPrecision::kDouble : model::floatPrecision::kSingle );
	pSummary.dResolutionX	= this->dCellResolutionX;
	pSummary.dResolutionY = this->dCellResolutionY;
	pSummary.bUseOptimizedBoundary = this->bUseOptimizedBoundary;
	pSummary.ulCouplingArraySize = this->ulCouplingArraySize;

	return pSummary;
}

/*
 *  Resting Boundary Conditions
 */
void	CDomainCartesian::resetBoundaryCondition()
{
	if (this->ucFloatSize == 4)
	{
		memset(fBoundaryValues, 0, sizeof(cl_float) * this->ulCellCount);
	}
	else {
		memset(dBoundaryValues, 0, sizeof(cl_double) * this->ulCellCount);
	}
}
