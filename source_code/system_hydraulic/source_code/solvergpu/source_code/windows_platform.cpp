/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */


#include "common.h"

#ifdef PLATFORM_WIN

char*	Util::getFileResource( const char * sName, const char * sType )
{
	HMODULE hModule = GetModuleHandle( NULL );


	if ( hModule == NULL )
		model::doError(
			"The module handle could not be obtained.",
			model::errorCodes::kLevelFatal,
			"char*	Util::getFileResource( const char * sName, const char * sType )",
			"HMODULE hModule = GetModuleHandle( NULL ) returned null"
		);

	HRSRC hResource = FindResource( hModule, static_cast<LPCSTR>(sName), static_cast<LPCSTR>(sType) );

	if ( hModule == NULL )
	{
		model::doError(
			"Could not obtain a requested resource.",
			model::errorCodes::kLevelWarning,
			"char*	Util::getFileResource( const char * sName, const char * sType )",
			"FindResource couldn't find resource [" + std::string(sName) + "] with type [" + std::string(sType) + "]"
		);
		return "";
	}

	HGLOBAL hData = LoadResource( hModule, hResource );

	if ( hData == NULL )
	{
		model::doError(
			"Could not load a requested resource value.",
			model::errorCodes::kLevelWarning,
			"char*	Util::getFileResource( const char * sName, const char * sType )",
			"LoadResource: Found the resource but couldn't Load it. Name: [" + std::string(sName) + "] .Type: [" + std::string(sType) + "]"
		);
		return "";
	}

	DWORD	dwSize		= SizeofResource( hModule, hResource );
	char*	cResource	= new char[ dwSize + 1 ];

	LPVOID	lpData		= LockResource( hData );
	char*	cSource		= static_cast<char *>(lpData);

	if ( lpData == NULL )
	{
		model::doError(
			"Could not obtain a pointer for a requested resource.",
			model::errorCodes::kLevelWarning,
			"char*	Util::getFileResource( const char * sName, const char * sType )",
			"LoadResource: Found the resource and loaded it But couldn't lock it. Name: [" + std::string(sName) + "] .Type: [" + std::string(sType) + "]"
		);
		return "";
	}

	memcpy( cResource, cSource, dwSize );
	cResource[ dwSize ] = 0;

	return cResource;
}

/*
 *  Get the system hostname
 */
void Util::getHostname(char* cHostname)
{
	std::strcpy(cHostname, "Unknown");
}

#endif

#ifdef PLATFORM_UNIX

inline std::string getOCLResourceFilename(std::string sID)
{
	const char* cID = sID.c_str();
	std::string sBaseDir = "";

	sBaseDir = "./";

	if (strcmp(cID, "CLUniversalHeader_H") == 0)
		return sBaseDir + "OpenCL/Executors/CLUniversalHeader.clh";

	if (strcmp(cID, "CLFriction_H") == 0)
		return sBaseDir + "Schemes/CLFriction.clh";

	if (strcmp(cID, "CLSchemeGodunov_H") == 0)
		return sBaseDir + "Schemes/CLSchemeGodunov.clh";

	if (strcmp(cID, "CLSchemeMUSCLHancock_H") == 0)
		return sBaseDir + "Schemes/CLSchemeMUSCLHancock.clh";

	if (strcmp(cID, "CLSchemeInertial_H") == 0)
		return sBaseDir + "Schemes/CLSchemeInertial.clh";

	if (strcmp(cID, "CLSolverHLLC_H") == 0)
		return sBaseDir + "Solvers/CLSolverHLLC.clh";

	if (strcmp(cID, "CLDynamicTimestep_H") == 0)
		return sBaseDir + "Schemes/CLDynamicTimestep.clh";

	if (strcmp(cID, "CLDomainCartesian_H") == 0)
		return sBaseDir + "Domain/Cartesian/CLDomainCartesian.clh";

	if (strcmp(cID, "CLSlopeLimiterMINMOD_H") == 0)
		return sBaseDir + "Schemes/Limiters/CLSlopeLimiterMINMOD.clh";

	if (strcmp(cID, "CLBoundaries_H") == 0)
		return sBaseDir + "Boundaries/CLBoundaries.clh";

	if (strcmp(cID, "CLVerifyDataStructure_C") == 0)
		return sBaseDir + "OpenCL/Executors/CLVerifyDataStructure.clc";

	if (strcmp(cID, "CLFriction_C") == 0)
		return sBaseDir + "Schemes/CLFriction.clc";

	if (strcmp(cID, "CLSchemeGodunov_C") == 0)
		return sBaseDir + "Schemes/CLSchemeGodunov.clc";

	if (strcmp(cID, "CLSchemeMUSCLHancock_C") == 0)
		return sBaseDir + "Schemes/CLSchemeMUSCLHancock.clc";

	if (strcmp(cID, "CLSchemeInertial_C") == 0)
		return sBaseDir + "Schemes/CLSchemeInertial.clc";

	if (strcmp(cID, "CLSolverHLLC_C") == 0)
		return sBaseDir + "Solvers/CLSolverHLLC.clc";

	if (strcmp(cID, "CLDynamicTimestep_C") == 0)
		return sBaseDir + "Schemes/CLDynamicTimestep.clc";

	if (strcmp(cID, "CLDomainCartesian_C") == 0)
		return sBaseDir + "Domain/Cartesian/CLDomainCartesian.clc";

	if (strcmp(cID, "CLSlopeLimiterMINMOD_C") == 0)
		return sBaseDir + "Schemes/Limiters/CLSlopeLimiterMINMOD.clc";

	if (strcmp(cID, "CLBoundaries_C") == 0)
		return sBaseDir + "Boundaries/CLBoundaries.clc";

	return "";
}

/*
 *  Fetch an embedded resource
 */
char* Util::getFileResource(const char* sName, const char* sType)
{
	std::string sFilename = getOCLResourceFilename(sName);

	if (sFilename.length() <= 0)
	{

		model::doError(
			"Requested an invalid resource.",
			model::errorCodes::kLevelWarning
		);
		return "";

	}

	try {

		std::ifstream pResourceFile(sFilename);
		std::stringstream ssResource;
		ssResource << pResourceFile.rdbuf();
		std::string sResource = ssResource.str();

		char* cResource = new char[sResource.length() + 1];
		memcpy(cResource, sResource.c_str(), sResource.length());
		cResource[sResource.length()] = 0;

		return cResource;

	}
	catch (std::exception)
	{
		model::doError(
			"Error loading a resource.",
			model::errorCodes::kLevelWarning
		);
		return "";
	}
}

/*
 *  Get the system hostname
 */
void Util::getHostname(char* cHostname)
{
	gethostname(cHostname, 255);
}
#endif