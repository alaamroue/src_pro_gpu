/*
 * Copyright (C) 2023 Alaa Mroue
 * This code is licensed under GPLv3. See LICENCE for more information.
 */

#include "common.h"
#include "CMultiGpuManager.h"


#include <vector>

/*
 *  Constructor
 */
CMultiGpuManager::CMultiGpuManager(void)
{
	this->numTotalDevices = 0;
	this->numCpu = 0;
	this->numGpu = 0;
	this->fetchHasError = false;

}


/*
 *  Destructor
 */
CMultiGpuManager::~CMultiGpuManager(void)
{

}

/*
 *  Init for Manager
 */
void CMultiGpuManager::initManager(void)
{
	this->fetchHasError = this->getPlatforms();
}

/*
 *  Check if we are forcing cpu
 */
bool CMultiGpuManager::getForceCpu(void)
{
	if (this->fetchHasError) {
		return true;
	}
	else {
	//do something
	}
}

/*
 *  Ascertain the number of, and store a pointer to each device
 *  available to us.
 */
bool CMultiGpuManager::getPlatforms(void)
{
    cl_int err;
    cl_uint numPlatforms, numDevices;

    // Get the number of available platforms
    err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (err != CL_SUCCESS) {
        std::cerr << "Error getting platform count: " << err << std::endl;
        return 1;
    }

    std::vector<cl_platform_id> platforms(numPlatforms);

    // Get the platform IDs
    err = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "Error getting platform IDs: " << err << std::endl;
        return 1;
    }

    std::cout << "Number of OpenCL platforms: " << numPlatforms << std::endl;

    for (cl_uint i = 0; i < numPlatforms; ++i) {
        // Get the number of available devices for each platform (GPU and CPU)
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
        if (err != CL_SUCCESS) {
            std::cerr << "Error getting device count for platform " << i << ": " << err << std::endl;
            continue;
        }

        std::vector<cl_device_id> devices(numDevices);

        // Get the device IDs
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);
        if (err != CL_SUCCESS) {
            std::cerr << "Error getting device IDs for platform " << i << ": " << err << std::endl;
            continue;
        }

        std::cout << "Number of devices for platform " << i << ": " << numDevices << std::endl;

        // Count the number of GPU and CPU devices
        int numGPUs = 0;
        int numCPUs = 0;

        for (cl_uint j = 0; j < numDevices; ++j) {
            cl_device_type deviceType;
            err = clGetDeviceInfo(devices[j], CL_DEVICE_TYPE, sizeof(cl_device_type), &deviceType, nullptr);
            if (err != CL_SUCCESS) {
                std::cerr << "Error getting device type for platform " << i << ", device " << j << ": " << err << std::endl;
                continue;
            }

            if (deviceType & CL_DEVICE_TYPE_GPU) {
                numGPUs++;
            }
            else if (deviceType & CL_DEVICE_TYPE_CPU) {
                numCPUs++;
            }
        }

        std::cout << "Number of GPU devices for platform " << i << ": " << numGPUs << std::endl;
        std::cout << "Number of CPU devices for platform " << i << ": " << numCPUs << std::endl;
    }

    return 0;
}
