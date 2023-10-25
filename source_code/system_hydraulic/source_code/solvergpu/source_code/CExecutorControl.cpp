/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */


// TODO: Remove this class entirely (we only deal with OpenCL
// executors for the purposes of this model now).

// Includes
#include "common.h"

#include "CExecutorControl.h"
#include "CExecutorControlOpenCL.h"

//Constructor
CExecutorControl::CExecutorControl(void)
{
	// This stub class is not enough to be ready
	// The child classes should change this value when they're
	// done initialising.
	this->setState( model::executorStates::executorError );
}

//Destructor
CExecutorControl::~CExecutorControl(void)
{
	// ...
}

//Create a new executor of the specified type (static func)
CExecutorControl*	CExecutorControl::createExecutor( unsigned char cType, CModel* cModel)
{
	switch ( cType )
	{
		case model::executorTypes::executorTypeOpenCL:
			return new CExecutorControlOpenCL(cModel); //Delete by pManager
		break;
	}

	return NULL;
}

//Is this executor ready to run models?
bool CExecutorControl::isReady( void )
{
	return this->state == model::executorStates::executorReady;
}

//Set the ready state of this executor
void CExecutorControl::setState( unsigned int iState )
{
	this->state = iState;
}

//Set the device filter to determine what types of device we can use to execute the model.
void CExecutorControl::setDeviceFilter( unsigned int uiFilters )
{
	this->deviceFilter = uiFilters;
}

//Return any current device filters
unsigned int CExecutorControl::getDeviceFilter()
{
	return this->deviceFilter;
}

