/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

#include <algorithm>

#include "common.h"
#include "CDomain.h"
#include "CDomainCartesian.h"
#include "CSchemePromaides.h"

using std::min;
using std::max;

/*
 *  Constructor
 */
CSchemePromaides::CSchemePromaides(void)
{
	// Scheme is loaded
	model::log->writeLine( "Promaides scheme loaded for execution on OpenCL platform." );

	// Default setup values
	this->bDebugOutput					= false;
	this->uiDebugCellX					= 100;
	this->uiDebugCellY					= 100;

	this->ucConfiguration				= model::schemeConfigurations::promaidesFormula::kCacheNone;
	this->ucCacheConstraints			= model::cacheConstraints::promaidesFormula::kCacheActualSize;
}

/*
 *  Destructor
 */
CSchemePromaides::~CSchemePromaides(void)
{
	this->releaseResources();
	model::log->writeLine( "The promaides formula scheme was unloaded from memory." );
}

/*
 *  Run all preparation steps
 */
void CSchemePromaides::prepareAll()
{
	// Clean any pre-existing OpenCL objects
	this->releaseResources();

	oclModel = new COCLProgram(
		cModel->getExecutor(),
		cModel->getExecutor()->getDevice()
	);

	// Run-time tracking values
	this->ulCurrentCellsCalculated = 0;
	this->dCurrentTimestep = this->dTimestep;
	this->dCurrentTime = 0;

	// Forcing single precision?
	this->oclModel->setForcedSinglePrecision(cModel->getFloatPrecision() == model::floatPrecision::kSingle);
	unsigned char ucFloatSize = (cModel->getFloatPrecision() == model::floatPrecision::kSingle ? sizeof(cl_double) : sizeof(cl_float));

	// OpenCL elements
	if (!this->prepare1OExecDimensions())
	{
		model::doError(
			"Failed to dimension 1st-order task elements. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	if (!this->prepare1OConstants())
	{
		model::doError(
			"Failed to allocate 1st-order constants. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	if (!this->prepareCode())
	{
		model::doError(
			"Failed to prepare model codebase. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	if (!this->prepare1OMemory())
	{
		model::doError(
			"Failed to create 1st-order memory buffers. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	if (!this->prepareGeneralKernels())
	{
		model::doError(
			"Failed to prepare general kernels. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}
	if (!this->preparePromaidesKernels())
	{
		model::doError(
			"Failed to prepare promaides kernels. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}


	this->logDetails();
	this->bReady = true;
}

/*
 *  Log the details and properties of this scheme instance.
 */
void CSchemePromaides::logDetails()
{
	model::log->writeDivide();
	unsigned short wColour = model::cli::colourInfoBlock;

	std::string sConfiguration = "Undefined";
	switch (this->ucConfiguration)
	{
	case model::schemeConfigurations::promaidesFormula::kCacheNone:
		sConfiguration = "Disabled";
		break;
	case model::schemeConfigurations::promaidesFormula::kCacheEnabled:
		sConfiguration = "Enabled";
		break;
	}

	model::log->writeLine("ProMaIDes SCHEME", true, wColour);
	model::log->writeLine("  Timestep mode:      " + (std::string)(this->bDynamicTimestep ? "Dynamic" : "Fixed"), true, wColour);
	model::log->writeLine("  Courant number:     " + (std::string)(this->bDynamicTimestep ? toStringExact(this->dCourantNumber) : "N/A"), true, wColour);
	model::log->writeLine("  Initial timestep:   " + Util::secondsToTime(this->dTimestep), true, wColour);
	model::log->writeLine("  Data reduction:     " + toStringExact(this->uiTimestepReductionWavefronts) + " divisions", true, wColour);
	model::log->writeLine("  Configuration:      " + sConfiguration, true, wColour);
	model::log->writeLine("  Friction effects:   " + (std::string)(this->bFrictionEffects ? "Enabled" : "Disabled"), true, wColour);
	model::log->writeLine("  Kernel queue mode:  " + (std::string)(this->bAutomaticQueue ? "Automatic" : "Fixed size"), true, wColour);
	model::log->writeLine((std::string)(this->bAutomaticQueue ? "  Initial queue:      " : "  Fixed queue:        ") + toStringExact(this->uiQueueAdditionSize) + " iteration(s)", true, wColour);
	model::log->writeLine("  Debug output:       " + (std::string)(this->bDebugOutput ? "Enabled" : "Disabled"), true, wColour);

	model::log->writeDivide();
}

/*
 *  Concatenate together the code for the different elements required
 */
bool CSchemePromaides::prepareCode()
{
	bool bReturnState = true;

	oclModel->appendCodeFromResource("CLDomainCartesian_H");
	oclModel->appendCodeFromResource("CLFriction_H");
	oclModel->appendCodeFromResource("CLDynamicTimestep_H");
	oclModel->appendCodeFromResource("CLSchemePromaides_H");
	oclModel->appendCodeFromResource("CLBoundaries_H");

	oclModel->appendCodeFromResource("CLDomainCartesian_C");
	oclModel->appendCodeFromResource("CLFriction_C");
	oclModel->appendCodeFromResource("CLDynamicTimestep_C");
	oclModel->appendCodeFromResource("CLSchemePromaides_C");
	oclModel->appendCodeFromResource("CLBoundaries_C");

	bReturnState = oclModel->compileProgram();

	return bReturnState;
}

/*
 *  Create kernels using the compiled program
 */
bool CSchemePromaides::preparePromaidesKernels()
{
	bool						bReturnState = true;
	CExecutorControlOpenCL* pExecutor = cModel->getExecutor();
	CDomain*					pDomain				= this->pDomain;
	COCLDevice*		pDevice				= pExecutor->getDevice();

	// --
	// Promaides scheme kernels
	// --


	oclKernelFullTimestep = oclModel->getKernel( "pro_cacheDisabled" );
	oclKernelFullTimestep->setGroupSize( this->ulNonCachedWorkgroupSizeX, this->ulNonCachedWorkgroupSizeY );
	oclKernelFullTimestep->setGlobalSize( this->ulNonCachedGlobalSizeX, this->ulNonCachedGlobalSizeY );
	COCLBuffer* aryArgsFullTimestep[] = { oclBufferTimestep, oclBufferCellBed, oclBufferCellStates, oclBufferCellStatesAlt, oclBufferCellManning, oclBufferUsePoleni, oclBuffer_opt_zxmax, oclBuffer_opt_cx, oclBuffer_opt_zymax, oclBuffer_opt_cy };
	oclKernelFullTimestep->assignArguments( aryArgsFullTimestep );


	return bReturnState;
}

/*
 *  Release all OpenCL resources consumed using the OpenCL methods
 */
void CSchemePromaides::releaseResources()
{
	this->bReady = false;

	model::log->writeLine("Releasing scheme resources held for OpenCL.");

	this->releasePromaidesResources();
	this->release1OResources();
}

/*
 *  Release all OpenCL resources consumed using the OpenCL methods
 */
void CSchemePromaides::releasePromaidesResources()
{
	this->bReady = false;

	model::log->writeLine("Releasing Promaides scheme resources held for OpenCL.");

	// Nothing to do?
}

/*
 *  Set the cache configuration to use
 */
void	CSchemePromaides::setCacheMode( unsigned char ucCacheMode )
{
	this->ucConfiguration = ucCacheMode;
}

/*
 *  Get the cache configuration in use
 */
unsigned char	CSchemePromaides::getCacheMode()
{
	return this->ucConfiguration;
}

/*
 *  Set the cache constraints
 */
void	CSchemePromaides::setCacheConstraints( unsigned char ucCacheConstraints )
{
	this->ucCacheConstraints = ucCacheConstraints;
}

/*
 *  Get the cache constraints
 */
unsigned char	CSchemePromaides::getCacheConstraints()
{
	return this->ucCacheConstraints;
}
