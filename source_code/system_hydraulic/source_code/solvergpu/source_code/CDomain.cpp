/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

#include "common.h"
#include "CDomain.h"
#include "CDomainCartesian.h"

#include "CScheme.h"
#include "COCLDevice.h"

//Constructor
CDomain::CDomain(void)
{
	this->pScheme			= NULL;
	this->pDevice			= NULL;
	this->bPrepared			= false;
	this->ucFloatSize		= 0;
	this->dMinFSL			= 9999.0;
	this->dMaxFSL			= -9999.0;
	this->dMinTopo			= 9999.0;
	this->dMaxTopo			= -9999.0;
	this->dMinDepth			= 9999.0;
	this->dMaxDepth			= -9999.0;
	this->uiRollbackLimit	= 999999999;


}

//Destructor
CDomain::~CDomain(void)
{
	if ( this->ucFloatSize == 4 )
	{
		delete [] this->fCellStates;
		delete [] this->fBedElevations;
		delete [] this->fManningValues;
		delete [] this->fOpt_zxmaxValues;
		delete [] this->fOpt_cxValues;
		delete [] this->fOpt_zymaxValues;
		delete [] this->fOpt_cyValues;
		if (this->getSummary().bUseOptimizedBoundary == false) {
			delete [] this->fBoundaryValues;
		}
		else {
			delete [] fCouplingValues;
			delete [] ulCouplingIDs;
		}
	} else if ( this->ucFloatSize == 8 ) {
		delete [] this->dCellStates;
		delete [] this->dBedElevations;
		delete [] this->dManningValues;
		delete [] this->dOpt_zxmaxValues;
		delete [] this->dOpt_cxValues;
		delete [] this->dOpt_zymaxValues;
		delete [] this->dOpt_cyValues;
		if (this->getSummary().bUseOptimizedBoundary == false) {
			delete[] this->dBoundaryValues;
		}
		else {
			delete[] dCouplingValues;
			delete[] ulCouplingIDs;
		}
	}
	delete[] this->bPoleniValues;

	if ( this->pScheme != NULL )     delete pScheme;
	delete [] this->cSourceDir;
	delete [] this->cTargetDir;

	model::log->logInfo("All domain memory has been released.");
}

//Creates an OpenCL memory buffer for the specified data store
void	CDomain::createStoreBuffers(
			void**			vArrayCellStates,
			void**			vArrayBedElevations,
			void**			vArrayManningCoefs,
			void**			vArrayBoundaryValues,
			void**			vArrayPoleniValues,
			void**			vArrayOpt_zxmax,
			void**			vArrayOpt_cx,
			void**			vArrayOpt_zymax,
			void**			vArrayOpt_cy,
			void**			vArrayCouplingIDs,
			void**			vArrayCouplingValues,
			unsigned char	ucFloatSize
		)
{
	if ( !bPrepared )
		prepareDomain();

	this->ucFloatSize = ucFloatSize;

	try {
		if ( ucFloatSize == sizeof( cl_float ) )
		{
			// Single precision
			this->fCellStates		= new cl_float4[ this->ulCellCount ];
			this->fBedElevations	= new cl_float[ this->ulCellCount ];
			this->fManningValues	= new cl_float[ this->ulCellCount ];
			this->fOpt_zxmaxValues	= new cl_float[ this->ulCellCount ];
			this->fOpt_cxValues		= new cl_float[ this->ulCellCount ];
			this->fOpt_zymaxValues	= new cl_float[ this->ulCellCount ];
			this->fOpt_cyValues		= new cl_float[ this->ulCellCount ];

			this->dCellStates		= (cl_double4*)( this->fCellStates );
			this->dBedElevations	= (cl_double*)( this->fBedElevations );
			this->dManningValues	= (cl_double*)( this->fManningValues );
			this->dOpt_zxmaxValues	= (cl_double*)( this->fOpt_zxmaxValues);
			this->dOpt_cxValues		= (cl_double*)( this->fOpt_cxValues);
			this->dOpt_zymaxValues	= (cl_double*)( this->fOpt_zymaxValues);
			this->dOpt_cyValues		= (cl_double*)( this->fOpt_cyValues);

			*vArrayCellStates	 = static_cast<void*>( this->fCellStates );
			*vArrayBedElevations = static_cast<void*>( this->fBedElevations );
			*vArrayManningCoefs	 = static_cast<void*>( this->fManningValues );
			*vArrayOpt_zxmax	 = static_cast<void*>( this->fOpt_zxmaxValues);
			*vArrayOpt_cx		 = static_cast<void*>( this->fOpt_cxValues);
			*vArrayOpt_zymax	 = static_cast<void*>( this->fOpt_zymaxValues);
			*vArrayOpt_cy		 = static_cast<void*>( this->fOpt_cyValues);

			if (this->getSummary().bUseOptimizedBoundary == false) {
				this->fBoundaryValues	= new cl_float[ this->ulCellCount ];
				this->dBoundaryValues = (cl_double*)(this->fBoundaryValues);
				*vArrayBoundaryValues = static_cast<void*>(this->fBoundaryValues);
			}else {
				this->fCouplingValues = new cl_float[this->getSummary().ulCouplingArraySize];
				this->dCouplingValues = (cl_double*)(this->fCouplingValues);
				*vArrayCouplingValues = static_cast<void*>(this->fCouplingValues);

				this->ulCouplingIDs = new cl_ulong[this->getSummary().ulCouplingArraySize];
				*vArrayCouplingIDs = static_cast<void*>(this->ulCouplingIDs);
			}
		} else {
			// Double precision
			this->dCellStates		= new cl_double4[ this->ulCellCount ];
			this->dBedElevations	= new cl_double[ this->ulCellCount ];
			this->dManningValues	= new cl_double[ this->ulCellCount ];
			this->dOpt_zxmaxValues  = new cl_double[ this->ulCellCount ];
			this->dOpt_cxValues		= new cl_double[ this->ulCellCount ];
			this->dOpt_zymaxValues  = new cl_double[ this->ulCellCount ];
			this->dOpt_cyValues		= new cl_double[ this->ulCellCount ];

			this->fCellStates		= (cl_float4*)( this->dCellStates );
			this->fBedElevations	= (cl_float*)( this->dBedElevations );
			this->fManningValues	= (cl_float*)( this->dManningValues );
			this->fOpt_zxmaxValues  = (cl_float*)( this->dOpt_zxmaxValues);
			this->fOpt_cxValues		= (cl_float*)( this->dOpt_cxValues);
			this->fOpt_zymaxValues  = (cl_float*)( this->dOpt_zymaxValues);
			this->fOpt_cyValues		= (cl_float*)( this->dOpt_cyValues);

			*vArrayCellStates		= static_cast<void*>( this->dCellStates );
			*vArrayBedElevations	= static_cast<void*>( this->dBedElevations );
			*vArrayManningCoefs		= static_cast<void*>( this->dManningValues );
			*vArrayOpt_zxmax		= static_cast<void*>( this->dOpt_zxmaxValues);
			*vArrayOpt_cx			= static_cast<void*>( this->dOpt_cxValues);
			*vArrayOpt_zymax		= static_cast<void*>( this->dOpt_zymaxValues);
			*vArrayOpt_cy			= static_cast<void*>( this->dOpt_cyValues);

			if (this->getSummary().bUseOptimizedBoundary == false) {
				this->dBoundaryValues = new cl_double[this->ulCellCount];
				this->fBoundaryValues = (cl_float*)(this->dBoundaryValues);
				*vArrayBoundaryValues = static_cast<void*>(this->fBoundaryValues);
			}
			else {
				this->dCouplingValues = new cl_double[this->getSummary().ulCouplingArraySize];
				this->fCouplingValues = (cl_float*)(this->dCouplingValues);
				*vArrayCouplingValues = static_cast<void*>(this->fCouplingValues);

				this->ulCouplingIDs = new cl_ulong[this->getSummary().ulCouplingArraySize];
				*vArrayCouplingIDs = static_cast<void*>(this->ulCouplingIDs);
			}

		}
		this->bPoleniValues			= new sUsePoleni[this->ulCellCount];
		*vArrayPoleniValues			= static_cast<void*>(this->bPoleniValues);
	}
	catch( const std::bad_alloc& e)
	{
		model::doError(
			"Memory allocation failed: std::bad_alloc",
			model::errorCodes::kLevelFatal,
			"void CDomain::createStoreBuffers( void**, void**, void**, void**, void**, void**, void**, void**, void**, void**, void**, unsigned char)",
			"The system ran out of memory. Try to run on a machine with more ram. Or use smaller floodplains"
		);
		return;
	}
}

//Populate all domain cells with default values. Shouldn't be required
void	CDomain::resetAllValues()
{
	model::log->logInfo( "Reseting heap domain data." );

	for( unsigned long i = 0; i < this->ulCellCount; i++ )
	{
		if ( this->ucFloatSize == 4 )
		{
			this->fCellStates[ i ].s[0]		= 0.0;	// Free-surface level
			this->fCellStates[ i ].s[1]		= 0.0;	// Maximum free-surface level
			this->fCellStates[ i ].s[2]		= 0.0;	// Discharge X
			this->fCellStates[ i ].s[3]		= 0.0;	// Discharge Y
			this->fBedElevations[ i ]		= 0.0;	// Bed elevation
			this->fManningValues[ i ]		= 0.0;	// Manning coefficient
			if (this->getSummary().bUseOptimizedBoundary == false) {
				this->fBoundaryValues[i] = 0.0;	// Boundary Values
			}

			this->fOpt_zxmaxValues[ i ]		= 0.0;	// Maxium elevation for poleni in X
			this->fOpt_cxValues[ i ]		= 0.0;	// Poleni factor in X
			this->fOpt_zymaxValues[ i ]		= 0.0;	// Maxium elevation for poleni in Y
			this->fOpt_cyValues[ i ]		= 0.0;	// Poleni factor in Y
		} else {
			this->dCellStates[ i ].s[0]		= 0.0;	// Free-surface level
			this->dCellStates[ i ].s[1]		= 0.0;	// Maximum free-surface level
			this->dCellStates[ i ].s[2]		= 0.0;	// Discharge X
			this->dCellStates[ i ].s[3]		= 0.0;	// Discharge Y
			this->dBedElevations[ i ]		= 0.0;	// Bed elevation
			this->dManningValues[ i ]		= 0.0;	// Manning coefficient
			if (this->getSummary().bUseOptimizedBoundary == false) {
				this->dBoundaryValues[ i ]	= 0.0;	// Boundary Values
			}
			this->dOpt_zxmaxValues[ i ]		= 0.0;	// Maxium elevation for poleni in X
			this->dOpt_cxValues[ i ]		= 0.0;	// Poleni factor in X
			this->dOpt_zymaxValues[ i ]		= 0.0;	// Maxium elevation for poleni in Y
			this->dOpt_cyValues[ i ]		= 0.0;	// Poleni factor in Y
		}

		this->bPoleniValues[i] = {false,false,false,false};					// Poleni flags
	}

	if (this->getSummary().bUseOptimizedBoundary == true) {
		for (unsigned long i = 0; i < this->getSummary().ulCouplingArraySize; i++)
		{
			if (this->ucFloatSize == 4)
			{
				this->fCouplingValues[i] = 0.0;	// Optimized Coupling Values
				this->ulCouplingIDs[i] = 0;// Optimized Coupling IDs
			}
			else {
				this->fCouplingValues[i] = 0.0;	// Optimized Coupling Values
				this->ulCouplingIDs[i] = 0;    // Optimized Coupling Values
			}
		}
	}
	model::log->logInfo("Reseting heap domain data Finished.");
}

//Sets the bed elevation for a given cell
void	CDomain::setBedElevation( unsigned long ulCellID, double dElevation )
{
	if ( this->ucFloatSize == 4 )
	{
		this->fBedElevations[ ulCellID ] = static_cast<float>( dElevation );
	} else {
		this->dBedElevations[ ulCellID ] = dElevation;
	}
}

//Sets the Manning coefficient for a given cell
void	CDomain::setManningCoefficient( unsigned long ulCellID, double dCoefficient )
{
	if ( this->ucFloatSize == 4 )
	{
		this->fManningValues[ ulCellID ] = static_cast<float>( dCoefficient );
	} else {
		this->dManningValues[ ulCellID ] = dCoefficient;
	}
}

//Sets a state variable for a given cell
void	CDomain::setStateValue( unsigned long ulCellID, unsigned char ucIndex, double dValue )
{
	if ( this->ucFloatSize == 4 )
	{
		this->fCellStates[ ulCellID ].s[ ucIndex ] = static_cast<float>( dValue );
	} else {
		this->dCellStates[ ulCellID ].s[ ucIndex ] = dValue;
	}
}

//Gets the bed elevation for a given cell
double	CDomain::getBedElevation( unsigned long ulCellID )
{
	if ( this->ucFloatSize == 4 ) 
		return static_cast<double>( this->fBedElevations[ ulCellID ] );
	return this->dBedElevations[ ulCellID ];
}

//Gets the Manning coefficient for a given cell
double	CDomain::getManningCoefficient( unsigned long ulCellID )
{
	if ( this->ucFloatSize == 4 ) 
		return static_cast<double>( this->fManningValues[ ulCellID ] );
	return this->dManningValues[ ulCellID ];
}

//Gets the Manning coefficient for a given cell
double	CDomain::getBoundaryCondition( unsigned long ulCellID )
{
	if ( this->ucFloatSize == 4 ) 
		return static_cast<double>( this->fBoundaryValues[ ulCellID ] );
	return this->dBoundaryValues[ ulCellID ];
}

//Gets a state variable for a given cell
double	CDomain::getStateValue( unsigned long ulCellID, unsigned char ucIndex )
{
	if ( this->ucFloatSize == 4 ) 
		return static_cast<double>( this->fCellStates[ ulCellID ].s[ ucIndex ] );
	return this->dCellStates[ ulCellID ].s[ ucIndex ];
}

//Handle initial conditions input data for a cell (usually from a raster dataset)
void	CDomain::handleInputData( 
			unsigned long	ulCellID, 
			double			dValue,
			unsigned char	ucValue,
			unsigned char	ucRounding
		)
{
	if ( !bPrepared )
		prepareDomain();

	switch( ucValue )
	{
	case model::rasterDatasets::dataValues::kBedElevation:
		this->setBedElevation( 
			ulCellID, 
			Util::round( dValue, ucRounding ) 
		);
		this->setStateValue( 
			ulCellID, 
			model::domainValueIndices::kValueFreeSurfaceLevel, 
			Util::round( dValue, ucRounding ) 
		);
		if ( dValue < dMinTopo && dValue != -9999.0 ) dMinTopo = dValue;
		if ( dValue > dMaxTopo && dValue != -9999.0 ) dMaxTopo = dValue;
		break;
	case model::rasterDatasets::dataValues::kFreeSurfaceLevel:
		this->setStateValue( 
			ulCellID,
			model::domainValueIndices::kValueFreeSurfaceLevel, 
			Util::round( dValue, ucRounding ) 
		);
		this->setStateValue( 
			ulCellID, 
			model::domainValueIndices::kValueMaxFreeSurfaceLevel, 
			Util::round( dValue, ucRounding ) 
		);
		if ( dValue - this->getBedElevation( ulCellID ) < dMinDepth && this->getBedElevation( ulCellID ) > -9999.0 && dValue > -9999.0 ) dMinDepth = dValue;
		if ( dValue - this->getBedElevation( ulCellID ) > dMaxDepth && this->getBedElevation( ulCellID ) > -9999.0 && dValue > -9999.0 ) dMaxDepth = dValue;
		if ( dValue < dMinFSL && this->getBedElevation( ulCellID ) > -9999.0 && dValue > -9999.0 ) dMinFSL = dValue;
		if ( dValue > dMaxFSL && this->getBedElevation( ulCellID ) > -9999.0 && dValue > -9999.0 ) dMaxFSL = dValue;
		break;
	case model::rasterDatasets::dataValues::kDepth:
		this->setStateValue( 
			ulCellID, 
			model::domainValueIndices::kValueFreeSurfaceLevel, 
			Util::round( ( this->getBedElevation( ulCellID ) + dValue ), ucRounding ) 
		);
		this->setStateValue( 
			ulCellID, 
			model::domainValueIndices::kValueMaxFreeSurfaceLevel, 
			Util::round( ( this->getBedElevation( ulCellID ) + dValue ), ucRounding ) 
		);
		if ( dValue + this->getBedElevation( ulCellID ) < dMinFSL && this->getBedElevation( ulCellID ) > -9999.0 && dValue > -9999.0 ) dMinFSL = dValue;
		if ( dValue + this->getBedElevation( ulCellID ) > dMaxFSL && this->getBedElevation( ulCellID ) > -9999.0 && dValue > -9999.0 ) dMaxFSL = dValue;
		if ( dValue < dMinDepth && this->getBedElevation( ulCellID ) > -9999.0 && dValue > -9999.0 ) dMinDepth = dValue;
		if ( dValue > dMaxDepth && this->getBedElevation( ulCellID ) > -9999.0 && dValue > -9999.0 ) dMaxDepth = dValue;
		break;
	case model::rasterDatasets::dataValues::kDisabledCells:
		// Cells are disabled using a free-surface level of -9999
		if ( dValue > 1.0 && dValue < 9999.0 )
		{
			this->setStateValue( 
				ulCellID, 
				model::domainValueIndices::kValueMaxFreeSurfaceLevel, 
				Util::round( ( -9999.0 ), ucRounding ) 
			);
		}
		break;
	case model::rasterDatasets::dataValues::kDischargeX:
		this->setStateValue( 
			ulCellID,
			model::domainValueIndices::kValueDischargeX, 
			Util::round( dValue, ucRounding ) 
		);
		break;
	case model::rasterDatasets::dataValues::kDischargeY:
		this->setStateValue( 
			ulCellID, 
			model::domainValueIndices::kValueDischargeY, 
			Util::round( dValue, ucRounding ) 
		);
		break;
	case model::rasterDatasets::dataValues::kVelocityX:
		this->setStateValue( 
			ulCellID, 
			model::domainValueIndices::kValueDischargeX, 
			Util::round( dValue * ( this->getStateValue( ulCellID, model::domainValueIndices::kValueFreeSurfaceLevel ) - this->getBedElevation( ulCellID ) ), ucRounding )
		);
		break;
	case model::rasterDatasets::dataValues::kVelocityY:
		this->setStateValue( 
			ulCellID, 
			model::domainValueIndices::kValueDischargeY, 
			Util::round( dValue * ( this->getStateValue( ulCellID, model::domainValueIndices::kValueFreeSurfaceLevel ) - this->getBedElevation( ulCellID ) ), ucRounding ) 
		);
		break;
	case model::rasterDatasets::dataValues::kManningCoefficient:
		this->setManningCoefficient( 
			ulCellID, 
			Util::round( dValue, ucRounding ) 
		);
		break;
	}
}


//Sets the Boundary values for a given cell
void	CDomain::setBoundaryCondition(unsigned long ulCellID, double dCoefficient)
{
	if (this->ucFloatSize == 4)
	{
		this->fBoundaryValues[ulCellID] = static_cast<float>(dCoefficient);
	}
	else {
		this->dBoundaryValues[ulCellID] = dCoefficient;
	}
}

//Sets the Boundary values for a given cell
void	CDomain::setOptimizedCouplingCondition(unsigned long index, double dCoefficient)
{
	if (this->ucFloatSize == 4)
	{
		this->fCouplingValues[index] = static_cast<float>(dCoefficient);
	}
	else {
		this->dCouplingValues[index] = dCoefficient;
	}
}

//Sets the Boundary values for a given cell
void	CDomain::setZxmax(unsigned long ulCellID, double dCoefficient)
{
	if (this->ucFloatSize == 4)
	{
		this->fOpt_zxmaxValues[ulCellID] = static_cast<float>(dCoefficient);
	}
	else {
		this->dOpt_zxmaxValues[ulCellID] = dCoefficient;
	}
}

//Sets the Boundary values for a given cell
void	CDomain::setcx(unsigned long ulCellID, double dCoefficient)
{
	if (this->ucFloatSize == 4)
	{
		this->fOpt_cxValues[ulCellID] = static_cast<float>(dCoefficient);
	}
	else {
		this->dOpt_cxValues[ulCellID] = dCoefficient;
	}
}

//Sets the Boundary values for a given cell
void	CDomain::setZymax(unsigned long ulCellID, double dCoefficient)
{
	if (this->ucFloatSize == 4)
	{
		this->fOpt_zymaxValues[ulCellID] = static_cast<float>(dCoefficient);
	}
	else {
		this->dOpt_zymaxValues[ulCellID] = dCoefficient;
	}
}

//Sets the Boundary values for a given cell
void	CDomain::setcy(unsigned long ulCellID, double dCoefficient)
{
	if (this->ucFloatSize == 4)
	{
		this->fOpt_cyValues[ulCellID] = static_cast<float>(dCoefficient);
	}
	else {
		this->dOpt_cyValues[ulCellID] = dCoefficient;
	}
}

//Sets the Optimized Coupling Ids
void	CDomain::setOptimizedCouplingID(unsigned long index, unsigned long ID)
{
	this->ulCouplingIDs[index] = ID;
}

//Sets the Poleni condition for a given cell in X
void	CDomain::setPoleniConditionX(unsigned long ulCellID, bool UsePoleniInX)
{
	//All values are already false by default, so we need to check which are true in the x direction, then change their neighbor to the east to true also in the -x direction
	// Todo: Alaa. Review why poleni is enabled on the borders anyways?
	if (UsePoleniInX) {

		unsigned long lIdxX = 0;
		unsigned long lIdxY = 0;
		getCellIndices(ulCellID, &lIdxX, &lIdxY);
		if (lIdxX < this->getSummary().ulColCount - 1) {
			this->bPoleniValues[ulCellID].usePoliniE = true;
			long ulCellID_Neigh_E = this->getNeighbourID(ulCellID, direction::east);
			this->bPoleniValues[ulCellID_Neigh_E].usePoliniW = true;
		}
	}

}

//Sets the Poleni condition for a given cell in Y
void	CDomain::setPoleniConditionY(unsigned long ulCellID, bool UsePoleniInY)
{
	//All values are already false by default, so we need to check which are true in the x direction, then change their neighbor to the west to true also in the -y direction
	// Todo: Alaa. Review why poleni is enabled on the borders anyways?
	if (UsePoleniInY) {

		unsigned long lIdxX = 0;
		unsigned long lIdxY = 0;
		getCellIndices(ulCellID, &lIdxX, &lIdxY);
		if (lIdxY < this->getSummary().ulRowCount - 1) {
			this->bPoleniValues[ulCellID].usePoliniN = true;
			long ulCellID_Neigh_N = this->getNeighbourID(ulCellID, direction::north);
			this->bPoleniValues[ulCellID_Neigh_N].usePoliniS = true;
		}
	}

}

//Calculate the total volume present in all of the cells
double	CDomain::getVolume()
{
	double dVolume = 0.0;

	// Stub
	return dVolume;
}

//Sets the scheme we're running on this domain
void	CDomain::setScheme( CScheme* pScheme )
{
	this->pScheme = pScheme;
}

//Gets the scheme we're running on this domain
CScheme*	CDomain::getScheme()
{
	return pScheme;
}

//Sets the device to use
void	CDomain::setDevice( COCLDevice* pDevice )
{
	this->pDevice = pDevice;
}

//Gets the scheme we're running on this domain
COCLDevice*	CDomain::getDevice()
{
	return this->pDevice;
}

//Gets the scheme we're running on this domain
CDomainBase::mpiSignalDataProgress	CDomain::getDataProgress()
{
	CDomainBase::mpiSignalDataProgress pResponse;
	CScheme *pScheme = getScheme();

	pResponse.uiDomainID = this->uiID;
	pResponse.dBatchTimesteps = pScheme->getAverageTimestep();
	pResponse.dCurrentTime = pScheme->getCurrentTime();
	pResponse.dCurrentTimestep = pScheme->getCurrentTimestep();
	pResponse.uiBatchSize = pScheme->getBatchSize();
	pResponse.uiBatchSkipped = pScheme->getIterationsSkipped();
	pResponse.uiBatchSuccessful = pScheme->getIterationsSuccessful();

	return pResponse;
}

//Fetch the code for a string description of an input/output
unsigned char	CDomain::getDataValueCode( char* cSourceValue )
{
	if ( strstr( cSourceValue, "dem" ) != NULL )		
		return model::rasterDatasets::dataValues::kBedElevation;
	if ( strstr( cSourceValue, "maxdepth" ) != NULL )		
	{
		return model::rasterDatasets::dataValues::kMaxDepth;
	}
	else if ( strstr( cSourceValue, "depth" ) != NULL )		
	{
		return model::rasterDatasets::dataValues::kDepth;
	}
	if ( strstr( cSourceValue, "disabled" ) != NULL )		
		return model::rasterDatasets::dataValues::kDisabledCells;
	if ( strstr( cSourceValue, "dischargex" ) != NULL )		
		return model::rasterDatasets::dataValues::kDischargeX;
	if ( strstr( cSourceValue, "dischargey" ) != NULL )		
		return model::rasterDatasets::dataValues::kDischargeY;
	if ( strstr( cSourceValue, "maxfsl" ) != NULL )
	{
		return model::rasterDatasets::dataValues::kMaxFSL;
	}
	else if ( strstr( cSourceValue, "fsl" ) != NULL )
	{
		return model::rasterDatasets::dataValues::kFreeSurfaceLevel;
	}
	if ( strstr( cSourceValue, "manningcoefficient" ) != NULL )		
		return model::rasterDatasets::dataValues::kManningCoefficient;
	if ( strstr( cSourceValue, "velocityx" ) != NULL )		
		return model::rasterDatasets::dataValues::kVelocityX;
	if ( strstr( cSourceValue, "velocityy" ) != NULL )		
		return model::rasterDatasets::dataValues::kVelocityY;
	if ( strstr( cSourceValue, "froude" ) != NULL )		
		return model::rasterDatasets::dataValues::kFroudeNumber;

	return 255;
}
