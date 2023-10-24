/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

#ifndef HIPIMS_DOMAIN_CARTESIAN_CDOMAINCARTESIAN_H_
#define HIPIMS_DOMAIN_CARTESIAN_CDOMAINCARTESIAN_H_

#include "CDomain.h"

/*
 *  DOMAIN CLASS
 *  CDomainCartesian
 *
 *  Stores the relevant information required for a
 *  domain based on a simple cartesian grid.
 */
class CDomainCartesian : public CDomain 
{

	public:

		CDomainCartesian( void );												// Constructor
		~CDomainCartesian( void );												// Destructor

		// Public variables
		// ...

		// Public functions
		// - Replacements for CDomain stubs
		virtual	unsigned char	getType()										{ return model::domainStructureTypes::kStructureCartesian; };	// Fetch a type code
		virtual	CDomainBase::DomainSummary getSummary();						// Fetch summary information for this domain
		bool			validateDomain( bool );									// Verify required data is available
		void			prepareDomain();										// Create memory structures etc.
		void			logDetails();											// Log details about the domain
		// - Specific to cartesian grids
		void			setCellResolution( double, double);						// Set cell resolution
		void			getCellResolution( double* , double*);					// Fetch cell resolution
		void			setUnits( char* );										// Set the units
		char*			getUnits();												// Get the units
		void			setProjectionCode( unsigned long );						// Set the EPSG projection code
		unsigned long	getProjectionCode();									// Get the EPSG projection code
		unsigned long	getRows();												// Get the number of rows in the domain
		unsigned long	getCols();												// Get the number of columns in the domain
		void			setRows(unsigned long);									// Fetch cell resolution
		void			setCols(unsigned long);									// Fetch cell resolution

		void			setUseOptimizedCoupling(bool);
		void			setOptimizedCouplingSize(unsigned long);
		bool			getUseOptimizedCoupling();
		unsigned long	getOptimizedCouplingSize();
		


		virtual unsigned long	getCellID( unsigned long, unsigned long );		// Get the cell ID using an X and Y index
		double			getVolume();											// Calculate the amount of volume in all the cells
		void			readBuffers_h_vx_vy(double**);							// Read GPU Buffers (All three Values)
		void			readBuffers_opt_h(double*);								// Read GPU Buffers (Water Depth: Surface Level - Bed Elevation)
		void			readBuffers_v_x(double*);								// Read GPU Buffers (Velocity in X)
		void			readBuffers_v_y(double*);								// Read GPU Buffers (Velocity in Y)
		void			resetBoundaryCondition();								// Resets boundary condition

		enum axis
		{
			kAxisX	= 0,
			kAxisY	= 1
		};

		enum edge
		{
			kEdgeN	= 0,
			kEdgeE	= 1,
			kEdgeS	= 2,
			kEdgeW	= 3
		};

		enum boundaryTreatment
		{
			kBoundaryOpen = 0,
			kBoundaryClosed = 1
		};

	private:

		// Private structures
		struct	sDataSourceInfo {
			char*			cSourceType;
			char*			cFileValue;
			unsigned char	ucValue;
		};
		struct	sDataTargetInfo
		{
			char*			cType;
			char*			cFormat;
			unsigned char	ucValue;
			std::string		sTarget;
		};

		// Private variables
		double			dCellResolutionX;
		double			dCellResolutionY;
		unsigned long	ulCouplingArraySize;
		bool			bUseOptimizedBoundary;
		unsigned long	ulRows;
		unsigned long	ulCols;

		// Private functions
		void			updateCellStatistics();										// Update the number of rows, cols, etc.

};

#endif
