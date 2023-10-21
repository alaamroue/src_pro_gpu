/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

#ifndef HIPIMS_DOMAIN_CDOMAINBASE_H_
#define HIPIMS_DOMAIN_CDOMAINBASE_H_

#include <vector>

#include "common.h"
#include "CL/opencl.h"

/*
 *  DOMAIN BASE CLASS
 *  CDomainBase
 *
 *  Core base class for domains, even ones which do not reside on this system.
 */
class CDomain;
class CDomainLink;
class CDomainBase
{

	public:

		CDomainBase(void);																			// Constructor
		~CDomainBase(void);																			// Destructor

		// Public structures
		struct DomainSummary
		{
			bool			bAuthoritative;
			unsigned int	uiDomainID;
			unsigned int	uiNodeID;
			unsigned int	uiLocalDeviceID;
			double			dResolutionX;
			double			dResolutionY;
			unsigned long	ulRowCount;
			unsigned long	ulColCount;
			unsigned char	ucFloatPrecision;
			unsigned long	ulCouplingArraySize;
			bool			bUseOptimizedBoundary;
		};

		struct mpiSignalDataProgress
		{
			unsigned int 	uiDomainID;
			double			dCurrentTimestep;
			double			dCurrentTime;
			double			dBatchTimesteps;
			cl_uint			uiBatchSkipped;
			cl_uint			uiBatchSuccessful;
			unsigned int	uiBatchSize;
		};

		// Public variables
		// ...

		// Public functions
		static		CDomainBase*	createDomain(unsigned char);									// Create a new domain of the specified type
		virtual		DomainSummary	getSummary();													// Fetch summary information for this domain
		virtual		bool			isRemote()				{ return true; };						// Is this domain on this node?
		virtual		unsigned char	getType()				{ return model::domainStructureTypes::kStructureInvalid; };	// Fetch a type code

		bool						isInitialised();												// X Is the domain ready to be used
		unsigned	long			getCellCount();													// X Return the total number of cells
		bool						isPrepared()			{ return bPrepared; }					// Is the domain prepared?
		unsigned int				getRollbackLimit()		{ return uiRollbackLimit; }				// How many iterations before a rollback is required?
		unsigned int				getID()					{ return uiID; }						// Get the ID number
		void						setID( unsigned int i ) { uiID = i; }							// Set the ID number
		unsigned int				getLinkCount()			{ return links.size(); }				// Get the number of links in this domain
		unsigned int				getDependentLinkCount()	{ return dependentLinks.size(); }		// Get the number of dependent links
		CDomainLink*				getLink(unsigned int i) { return links[i]; };					// Fetch a link
		CDomainLink*				getDependentLink(unsigned int i) { return dependentLinks[i]; };	// Fetch a dependent link
		CDomainLink*				getLinkFrom(unsigned int);										// Fetch a link with a specific domain
		bool						sendLinkData();													// Send link data to other nodes
		bool						isLinkSetAtTime( double );										// Are all this domain's links at the specified time?
		void						clearLinks()			{ links.clear(); dependentLinks.clear(); }	// Remove pre-existing links
		void						addLink(CDomainLink*);											// Add a new link to another domain
		void						addDependentLink(CDomainLink*);									// Add a new dependent link to another domain
		void						markLinkStatesInvalid();										// When a rollback is initiated, link data becomes invalid
		void						setRollbackLimit();												// Automatically identify a rollback limit
		void						setRollbackLimit( unsigned int i ) { uiRollbackLimit = i; }		// Set the number of iterations before a rollback is required
		virtual unsigned long		getCellID(unsigned long, unsigned long);						// Get the cell ID using an X and Y index
		virtual void				getCellIndices(unsigned long ulID, unsigned long* lIdxX, unsigned long* lIdxY);
		virtual unsigned long		getNeighbourID(unsigned long ulCellID, unsigned char  ucDirection);
		virtual mpiSignalDataProgress getDataProgress()		{ return pDataProgress; };				// Fetch some data on this domain's progress
		virtual void 				setDataProgress( mpiSignalDataProgress a )	{ pDataProgress = a; };	// Set some data on this domain's progress

	protected:

		// Private variables
		bool				bPrepared;																// Is the domain prepared?
		unsigned int		uiID;																	// Domain ID
		unsigned int		uiRollbackLimit;														// Iterations after which a rollback is required
		unsigned long		ulCellCount;															// Total number of cells
		mpiSignalDataProgress pDataProgress;														// Data on this domain's progress
		std::vector<CDomainLink*>	links;															// Vector of domain links
		std::vector<CDomainLink*>	dependentLinks;													// Vector of dependent domain links

	enum direction
	{
		north = 0,
		east = 1,
		south = 2,
		west = 3
	};
};

#endif
