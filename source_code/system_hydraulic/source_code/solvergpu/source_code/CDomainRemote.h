/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

#ifndef HIPIMS_DOMAIN_CDOMAINREMOTE_H_
#define HIPIMS_DOMAIN_CDOMAINREMOTE_H_

#include "CL/opencl.h"
#include "CDomainBase.h"

/*
 *  DOMAIN CLASS
 *  CDomainRemote
 *
 *  Stores relevant details for a domain which resides elsewhere in the MPI comm.
 */
class CDomainRemote : public CDomainBase
{
	public:

		CDomainRemote(void);																// Constructor
		~CDomainRemote(void);																// Destructor

		// Public variables
		// ...

		// Public functions
		virtual	unsigned char				getType()				{ return model::domainStructureTypes::kStructureRemote; };	// Fetch a type code
		virtual	bool						isRemote()				{ return true; };		// Is this domain on this node?
		virtual	CDomainBase::DomainSummary	getSummary();									// Fetch summary information for this domain
		void								setSummary(CDomainBase::DomainSummary a) { pSummary = a; };

	protected:

		// None

	private:

		CDomainBase::DomainSummary			pSummary;										// Domain summary info received from the remote node
};

#endif
