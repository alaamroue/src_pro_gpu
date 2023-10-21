/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

#ifndef HIPIMS_DOMAIN_CDOMAIN_H_
#define HIPIMS_DOMAIN_CDOMAIN_H_

#include "CL/opencl.h"
#include "CDomainBase.h"

// TODO: Make a CLocation class
class CDomainCartesian;
class CBoundaryMap;
class COCLDevice;
class COCLBuffer;
class CScheme;

/*
 *  DOMAIN CLASS
 *  CDomain
 *
 *  Stores relevant details for the computational domain and
 *  common operations required.
 */
class CDomain : public CDomainBase
{

	public:

		CDomain( void );																			// Constructor
		~CDomain( void );																			// Destructor

		// Public variables
		// ...

		// Public functions
		virtual		bool			isRemote()				{ return false; };						// Is this domain on this node?
		virtual		bool			validateDomain( bool ) = 0;										// Verify required data is available
		virtual		void			prepareDomain() = 0;											// Create memory structures etc.
		virtual		void			logDetails() = 0;												// Log details about the domain
		virtual		void			updateCellStatistics() = 0;										// Update the total number of cells calculation
		virtual		double**		readBuffers_h_vx_vy() = 0;										// Read the gpu buffers to a double**
		virtual		double*			readBuffers_opt_h() = 0;										// Read the gpu buffers water depth to a double*
		virtual		double*			readBuffers_v_x() = 0;											// Read the gpu buffers velocity in x to a double*
		virtual		double*			readBuffers_v_y() = 0;											// Read the gpu buffers velocity in y to a double*
		void						createStoreBuffers( void**, void**, void**, void**, void**, void**, void**, void**, void**, void**, void**, unsigned char );	// Allocates memory and returns pointers to the three arrays
		void						initialiseMemory();												// Populate cells with default values
		void						resetAllValues();												// Reset cell values to default values
		void						handleInputData( unsigned long, double, unsigned char, unsigned char );	// Handle input data for varying state/static cell variables 
		void						setBedElevation( unsigned long, double );						// Sets the bed elevation for a cell
		void						setManningCoefficient( unsigned long, double );					// Sets the manning coefficient for a cell
		void						setBoundaryCondition( unsigned long, double );					// Sets the boundary coefficient for a cell
		void						setOptimizedCouplingCondition( unsigned long, double );					// Sets the optimized coupling boundary coefficient for a cell

		void						setZxmax( unsigned long, double );					// Sets the boundary coefficient for a cell
		void						setcx( unsigned long, double );					// Sets the boundary coefficient for a cell
		void						setZymax( unsigned long, double );					// Sets the boundary coefficient for a cell
		void						setcy( unsigned long, double );					// Sets the boundary coefficient for a cell

		void						setOptimizedCouplingID(unsigned long, unsigned long);					// Sets the boundary coefficient for a cell

		void						setPoleniConditionX( unsigned long, bool );						// Sets the poleni conditon in x for a cell
		void						setPoleniConditionY( unsigned long, bool );						// Sets the poleni conditon in y for a cell
		void						setStateValue( unsigned long, unsigned char, double );			// Sets a state variable
		bool						isDoublePrecision() { return ( ucFloatSize == 8 ); };			// Are we using double-precision?
		double						getBedElevation( unsigned long );								// Gets the bed elevation for a cell
		double						getManningCoefficient( unsigned long );							// Gets the manning coefficient for a cell
		double						getBoundaryCondition( unsigned long );							// Gets the manning coefficient for a cell
		double						getStateValue( unsigned long, unsigned char );					// Gets a state variable
		double						getMaxFSL()				{ return dMaxFSL; }						// Fetch the maximum FSL in the domain
		double						getMinFSL()				{ return dMinFSL; }						// Fetch the minimum FSL in the domain
		virtual double				getVolume();													// Calculate the total volume in all the cells
		unsigned int				getID()					{ return uiID; }						// Get the ID number
		void						setID( unsigned int i ) { uiID = i; }							// Set the ID number
		virtual mpiSignalDataProgress getDataProgress();											// Fetch some data on this domain's progress

		void						setScheme( CScheme* );											// Set the scheme running for this domain
		CScheme*					getScheme();													// Get the scheme running for this domain
		void						setDevice( COCLDevice* );										// Set the device responsible for running this domain
		COCLDevice*					getDevice();													// Get the device responsible for running this domain


	protected:

		// Private variables
		unsigned char		ucFloatSize;															// Size of floats used for cell data (bytes)
		char*				cSourceDir;																// Data source dir
		char*				cTargetDir;																// Output target dir

		cl_double4*			dCellStates;															// Heap for cell state data
		cl_double*			dBedElevations;															// Heap for bed elevations
		cl_double*			dManningValues;															// Heap for manning values
		cl_double*			dBoundaryValues;														// Heap for boundary values
		cl_double*			dOpt_zxmaxValues;														// Heap for opt_zxmax values
		cl_double*			dOpt_cxValues;															// Heap for opt_cx values
		cl_double*			dOpt_zymaxValues;														// Heap for opt_zymax values
		cl_double*			dOpt_cyValues;															// Heap for opt_cy values
		cl_double*			dCouplingValues;														// Heap for optimized coupling values

		cl_float4*			fCellStates;															// Heap for cell state date (single)
		cl_float*			fBedElevations;															// Heap for bed elevations (single)
		cl_float*			fManningValues;															// Heap for manning values (single)
		cl_float*			fBoundaryValues;														// Heap for boundary values (single)
		cl_float*			fOpt_zxmaxValues;														// Heap for opt_zxmax values (single)
		cl_float*			fOpt_cxValues;															// Heap for opt_cx values (single)
		cl_float*			fOpt_zymaxValues;														// Heap for opt_zymax values (single)
		cl_float*			fOpt_cyValues;															// Heap for opt_cy values (single)
		cl_float*			fCouplingValues;														// Heap for optimized coupling values (single)

		sUsePoleni*			bPoleniValues;															// Heap for Struct of Poleni values
		cl_ulong*			ulCouplingIDs;															// Heap for optimized coupling IDs

		cl_double			dMinFSL;																// Min and max FSLs in the domain used for rendering
		cl_double			dMaxFSL;
		cl_double			dMinTopo;																// Min and max topographic levels above datum
		cl_double			dMaxTopo;
		cl_double			dMinDepth;																// Min and max depths 
		cl_double			dMaxDepth;

		CScheme*			pScheme;																// Scheme we are running for this particular domain
		COCLDevice*			pDevice;																// Device responsible for running this domain

		// Private functions
		unsigned char		getDataValueCode( char* );												// Get a raster dataset code from text description
};

#endif
