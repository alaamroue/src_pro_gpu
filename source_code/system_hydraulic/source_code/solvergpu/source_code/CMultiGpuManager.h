/*
 * Copyright (C) 2023 Alaa Mroue
 * This code is licensed under GPLv3. See LICENCE for more information.
 */

#pragma once

#include "CL/opencl.h"

class CMultiGpuManager{

public:

	CMultiGpuManager(void);								// Constructor
	~CMultiGpuManager(void);							// Destructor

	// Public Functions
	void	initManager();							// Input data to gpu manager
	bool	getForceCpu();							// Checks if there are any gpu devices available if not return false
	int		getDeviceBasedonFloodplainNumber(int);	// Return device to select based on floodplainID

private:

	// Private Varialbles
	int numTotalDevices;
	int numCpu;
	int numGpu;
	bool fetchHasError;
	char* errorMessage;
	cl_uint					clPlatformCount;									// Number of platforms

	bool					getPlatforms(void);									// Discovers the platforms available
	void					logPlatforms(void);									// Write platform details to the log
	char*					getPlatformInfo(unsigned int, cl_platform_info);	// Fetches information about the platform
	cl_device_type			getDeviceType(cl_device_id deviceId);
	void*					getDeviceInfo(cl_device_id, cl_device_info );


	// Private structs
	struct		sPlatformInfo
	{
		char* cProfile;
		char* cVersion;
		char* cName;
		char* cVendor;
		char* cExtensions;
		cl_uint				uiDeviceCount;
	};

	// Private variables
	sPlatformInfo* platformInfo;					// Platform details
	cl_platform_id* clPlatforms;					// Array of OpenCL platforms

};