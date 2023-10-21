/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

#ifndef HIPIMS_DOMAIN_CDOMAINMANAGER_H_
#define HIPIMS_DOMAIN_CDOMAINMANAGER_H_

#include "CL/opencl.h"
#include <vector>

class CDomainBase;
class CDomain;
class CDomainCartesian;
class COCLDevice;
class CScheme;
class CRasterDataset;

/*
 *  DOMAIN MANAGER CLASS
 *  CDomainManager
 *
 *  Manages the domains and the data in each.
 */
class CDomainManager
{

	public:

		CDomainManager( void );																		// Constructor
		~CDomainManager( void );																	// Destructor

		// Public structures
		struct Bounds
		{
			double N, E, S, W;
		};

		// Public variables
		// ...

		// Public functions
		bool					isDomainLocal(unsigned int);										// Is this domain local to this node?
		std::vector<CDomainBase*>* getDomainBaseVector();											// Fetch a domain base vector
		CDomainBase*			getDomainBase(unsigned int);										// Fetch a domain base by ID
		CDomain*				getDomain( unsigned int );											// Fetch a domain by ID
		CDomain*				getDomain( double, double );										// Fetch a domain by coordinate
		unsigned int			getDomainCount();													// Get the total number of domains
		Bounds					getTotalExtent();													// Fetch the total extent of all domains
		//bool					addRasterDataset( CRasterDataset*, unsigned char );					// Load a raster dataset
		//bool					addConstantValue( unsigned char, double );							// Set a constant value for all domains
		void					setSyncMethod(unsigned char);										// Set sync method
		unsigned char			getSyncMethod();													// Fetch sync method
		void					setSyncBatchSpares(unsigned int);									// Set batch spares to aim for
		unsigned int			getSyncBatchSpares();												// Fetch batch spares to aim for
		bool					isSetContiguous();													// Are all of the domains contiguous
		bool					isSetReady();														// Is the set of domains ready?
		void					logDetails();														// Spit out some information
		void					generateLinks();													// Generate domain link records

	protected:

		// Private variables
		std::vector<CDomainBase*> domains;															// Vector of all the domains we hold
		unsigned char			ucSyncMethod;														// Method of domain synchronisation
		unsigned int			uiSyncSpareIterations;												// Aim for # spare iterations when synchronising

		// Private functions
		CDomainBase*			createNewDomain( unsigned char );									// Add a new domain

};

#endif
