/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

// Includes
#include "common.h"
#include "CL/opencl.h"
#include "CModel.h"
#include "CExecutorControl.h"
#include "CExecutorControlOpenCL.h"
#include "COCLDevice.h"

/*
 *  Constructor
 */
COCLDevice::COCLDevice( cl_device_id clDevice, unsigned int iPlatformID, unsigned int iDeviceNo, CExecutorControlOpenCL* executor, CModel* cModel)
{
	// Store the device and platform ID
	this->uiPlatformID			= iPlatformID;
	this->uiDeviceNo			= iDeviceNo + 1;
	this->callBackData.DeviceNumber = &this->uiDeviceNo;
	this->clDevice				= clDevice;
	this->execController		= executor;
	this->callBackData.Executor = this->execController;
	this->bForceSinglePrecision	= false;
	this->bErrored				= false;
	this->bBusy					= false;
	this->clMarkerEvent			= NULL;
	this->cModel				= cModel;
	this->callBackData.cModel	= this->cModel;

	model::log->logInfo("Querying the suitability of a discovered device.");

	this->getAllInfo();
	this->createQueue();
}

/*
 *  Destructor
 */
COCLDevice::~COCLDevice(void)
{
	clFinish(this->clQueue);
	clReleaseCommandQueue(this->clQueue);
	clReleaseContext(this->clContext);

	delete[] this->clDeviceMaxWorkItemSizes;
	delete[] this->clDeviceName;
	delete[] this->clDeviceCVersion;
	delete[] this->clDeviceProfile;
	delete[] this->clDeviceVendor;
	delete[] this->clDeviceOpenCLVersion;
	delete[] this->clDeviceOpenCLDriver;

	model::log->logInfo("An OpenCL device has been released (#" + toStringExact(this->uiDeviceNo) + ").");
}

/*
 *  Obtain the size and value for a device info field
 */
void COCLDevice::getAllInfo()
{
	void* vMemBlock;

	// This is messy, but at least it doesn't memory leak...
	// TODO: This should really use a template function for getDeviceInfo. That'd be sensible...
	vMemBlock = this->getDeviceInfo(CL_DEVICE_ADDRESS_BITS);
	this->clDeviceAddressSize = *static_cast<cl_uint*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_AVAILABLE);
	this->clDeviceAvailable = *static_cast<cl_bool*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_COMPILER_AVAILABLE);
	this->clDeviceCompilerAvailable = *static_cast<cl_bool*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_ERROR_CORRECTION_SUPPORT);
	this->clDeviceErrorCorrection = *static_cast<cl_bool*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_EXECUTION_CAPABILITIES);
	this->clDeviceExecutionCapability = *static_cast<cl_device_exec_capabilities*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_GLOBAL_MEM_CACHE_SIZE);
	this->clDeviceGlobalCacheSize = *static_cast<cl_ulong*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_GLOBAL_MEM_CACHE_TYPE);
	this->clDeviceGlobalCacheType = *static_cast<cl_device_mem_cache_type*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_GLOBAL_MEM_SIZE);
	this->clDeviceGlobalSize = *static_cast<cl_ulong*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_LOCAL_MEM_SIZE);
	this->clDeviceLocalSize = *static_cast<cl_ulong*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_LOCAL_MEM_TYPE);
	this->clDeviceLocalType = *static_cast<cl_device_local_mem_type*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_MAX_CLOCK_FREQUENCY);
	this->clDeviceClockFrequency = *static_cast<cl_uint*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_MAX_COMPUTE_UNITS);
	this->clDeviceComputeUnits = *static_cast<cl_uint*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_MAX_CONSTANT_ARGS);
	this->clDeviceMaxConstants = *static_cast<cl_uint*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE);
	this->clDeviceMaxConstantSize = *static_cast<cl_ulong*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_MAX_MEM_ALLOC_SIZE);
	this->clDeviceMaxMemAlloc = *static_cast<cl_ulong*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_GLOBAL_MEM_SIZE);
	this->clDeviceGlobalMemSize = *static_cast<cl_ulong*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_MAX_PARAMETER_SIZE);
	this->clDeviceMaxParamSize = *static_cast<size_t*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE);
	this->clDeviceMaxWorkGroupSize = *static_cast<size_t*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS);
	this->clDeviceMaxWorkItemDims = *static_cast<cl_uint*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_PROFILING_TIMER_RESOLUTION);
	this->clDeviceTimerResolution = *static_cast<size_t*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_QUEUE_PROPERTIES);
	this->clDeviceQueueProperties = *static_cast<cl_command_queue_properties*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_SINGLE_FP_CONFIG);
	this->clDeviceSingleFloatConfig = *static_cast<cl_device_fp_config*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_DOUBLE_FP_CONFIG);
	this->clDeviceDoubleFloatConfig = *static_cast<cl_device_fp_config*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_TYPE);
	this->clDeviceType = *static_cast<cl_device_type*>(vMemBlock);
	delete[] vMemBlock;
	vMemBlock = this->getDeviceInfo(CL_DEVICE_MEM_BASE_ADDR_ALIGN);
	this->clDeviceAlignBits = *static_cast<cl_uint*>(vMemBlock);
	delete[] vMemBlock;

	this->clDeviceMaxWorkItemSizes = (size_t*)this->getDeviceInfo(CL_DEVICE_MAX_WORK_ITEM_SIZES);
	this->clDeviceName = (char*)this->getDeviceInfo(CL_DEVICE_NAME);
	this->clDeviceCVersion = (char*)this->getDeviceInfo(CL_DEVICE_OPENCL_C_VERSION);
	this->clDeviceProfile = (char*)this->getDeviceInfo(CL_DEVICE_PROFILE);
	this->clDeviceVendor = (char*)this->getDeviceInfo(CL_DEVICE_VENDOR);
	this->clDeviceOpenCLVersion = (char*)this->getDeviceInfo(CL_DEVICE_VERSION);
	this->clDeviceOpenCLDriver = (char*)this->getDeviceInfo(CL_DRIVER_VERSION);
}

/*
 *  Obtain the size and value for a device info field
 */
void* COCLDevice::getDeviceInfo(cl_device_info clInfo)
{
	cl_int		iErrorID;
	size_t		clSize;

	iErrorID = clGetDeviceInfo(
		this->clDevice,
		clInfo,
		NULL,
		NULL,
		&clSize
	);

	if (iErrorID != CL_SUCCESS)
		clSize = 1;

	char* cValue = new char[clSize + 1];

	iErrorID = clGetDeviceInfo(
		this->clDevice,
		clInfo,
		clSize,
		cValue,
		NULL
	);

	if (iErrorID != CL_SUCCESS)
		cValue[0] = 0;

	return cValue;
}

/*
 *  Write details of this device to the log
 */
void COCLDevice::logDevice()
{
	CLog* pLog = model::log;
	std::string	sPlatformNo;

	pLog->writeDivide();

	unsigned short	wColour = model::cli::colourInfoBlock;

	std::string sDeviceType = " UNKNOWN DEVICE TYPE";
	if (this->clDeviceType & CL_DEVICE_TYPE_CPU)			sDeviceType = " CENTRAL PROCESSING UNIT";
	if (this->clDeviceType & CL_DEVICE_TYPE_GPU)			sDeviceType = " GRAPHICS PROCESSING UNIT";
	if (this->clDeviceType & CL_DEVICE_TYPE_ACCELERATOR)	sDeviceType += " AND ACCELERATOR";

	std::string sDoubleSupport = "Not supported";
	if (this->isDoubleCompatible())
		sDoubleSupport = "Available";

	std::stringstream ssGroupDimensions;
	ssGroupDimensions << "[" << this->clDeviceMaxWorkItemSizes[0] <<
		", " << this->clDeviceMaxWorkItemSizes[1] <<
		", " << this->clDeviceMaxWorkItemSizes[2] << "]";

	pLog->logInfo("#" + toStringExact(this->uiDeviceNo) + sDeviceType);

	pLog->logInfo("  Suitability:       " + (std::string)(this->clDeviceAvailable ? "Available" : "Unavailable") + ", " + (std::string)(this->clDeviceCompilerAvailable ? "Compiler found" : "No compiler available"));
	pLog->logInfo("  Processor type:    " + std::string(this->clDeviceName));
	pLog->logInfo("  Vendor:            " + std::string(this->clDeviceVendor));
	pLog->logInfo("  OpenCL driver:     " + std::string(this->clDeviceOpenCLDriver));
	pLog->logInfo("  Compute units:     " + toStringExact(this->clDeviceComputeUnits));
	pLog->logInfo("  Profile:           " + (std::string)(std::string(this->clDeviceProfile).compare("FULL_PROFILE") == 0 ? "Full" : "Embedded"));
	pLog->logInfo("  Clock speed:       " + toStringExact(this->clDeviceClockFrequency) + " MHz");
	pLog->logInfo("  Memory:            " + toStringExact((unsigned int)(this->clDeviceGlobalMemSize / 1024 / 1024)) + " Mb");
	pLog->logInfo("  OpenCL C:          " + std::string(this->clDeviceOpenCLVersion));
	pLog->logInfo("  Max global size:   " + toStringExact(this->clDeviceGlobalSize));
	pLog->logInfo("  Max group items:   " + toStringExact(this->clDeviceMaxWorkGroupSize));
	pLog->logInfo("  Max group:         " + ssGroupDimensions.str());
	pLog->logInfo("  Max constant args: " + toStringExact(this->clDeviceMaxConstants));
	pLog->logInfo("  Max allocation:    " + toStringExact(this->clDeviceMaxMemAlloc / 1024 / 1024) + "MB");
	pLog->logInfo("  Max argument size: " + toStringExact(this->clDeviceMaxParamSize / 1024) + "kB");
	pLog->logInfo("  Double precision:  " + sDoubleSupport);

	pLog->writeDivide();
}

/*
 *  Create the context and command queue for this device
 */
void COCLDevice::createQueue()
{
	cl_int	iErrorID;

	if (!this->isSuitable())
	{
		model::doError(
			"Unsuitable device discovered. May be in use already.",
			model::errorCodes::kLevelWarning,
			"void COCLDevice::createQueue()",
			"The selected device is busy. Check for other programs using the gpu."
		);
		return;
	}

	model::log->logInfo("Creating an OpenCL device context and command queue.");

	this->clContext = clCreateContext(
		NULL,						// Properties (nothing special required) 
		1,							// Number of devices
		&this->clDevice,			// Device
		NULL,						// Error handling callback
		NULL,						// User data argument for the error handling callback
		&iErrorID					// Error ID pointer
	);

	if (iErrorID != CL_SUCCESS)
	{
		model::doError(
			"Error creating device context. Got an error: [" + std::to_string(iErrorID) + "] from clCreateContext using device [" + this->clDeviceName + "]",
			model::errorCodes::kLevelWarning,
			"void COCLDevice::createQueue()",
			"Try to restart the program or PC."
		);
		return;
	}

	this->clQueue = clCreateCommandQueue(
		this->clContext,
		this->clDevice,
		0,
		&iErrorID
	);

	if (iErrorID != CL_SUCCESS)
	{
		model::doError(
			"Error creating device command queue. Got an error: [" + std::to_string(iErrorID) + "] from clCreateCommandQueue using device [" + this->clDeviceName + "]",
			model::errorCodes::kLevelWarning,
			"void COCLDevice::createQueue()",
			"Try to restart the program or PC."
		);
		return;
	}

	model::log->logInfo("Command queue created for device successfully.");
}

/*
 *  Is this device suitable for use?
 */
bool COCLDevice::isSuitable()
{
	if (!this->clDeviceAvailable)
	{
		model::log->logInfo("Device is not available.");
		return false;
	}

	if (!this->clDeviceCompilerAvailable)
	{
		model::log->logInfo("No compiler is available.");
		return false;
	}

	// Other restrictions, like the memory and work group sizes?
	// ...

	return true;
}

/*
 *  Is this device ready for use?
 */
bool COCLDevice::isReady()
{
	if (!this->isSuitable())
	{
		model::log->logInfo("Device is not considered suitable.");
		return false;
	}

	if (!this->clContext ||
		!this->clQueue ||
		this->bErrored == true)
	{
		model::log->logInfo("No context, queue or an error occured on device.");
		if (!this->clContext)
			model::log->logInfo(" - No context");
		if (!this->clQueue)
			model::log->logInfo(" - No command queue");
		if (!this->bErrored)
			model::log->logInfo(" - Device error");
		return false;
	}

	return true;
}

/*
 *  Is this device filtered, and therefore should be ignored for execution steps?
 */
bool COCLDevice::isFiltered()
{
	if (!(execController->getDeviceFilter() & model::filters::devices::devicesGPU) &&
		this->clDeviceType == CL_DEVICE_TYPE_GPU)
		return true;
	if (!(execController->getDeviceFilter() & model::filters::devices::devicesCPU) &&
		this->clDeviceType == CL_DEVICE_TYPE_CPU)
		return true;
	if (!(execController->getDeviceFilter() & model::filters::devices::devicesAPU) &&
		this->clDeviceType == CL_DEVICE_TYPE_ACCELERATOR)
		return true;

	return false;
}

/*
 *  Enqueue a buffer command to synchronise threads
 */
void	COCLDevice::queueBarrier()
{
	#ifdef USE_SIMPLE_ARCH_OPENCL
	// Causes crashes... for some reason... Review later.
	return;
	#endif
	clEnqueueBarrier(this->clQueue);
}

/*
 *  Block program execution until all commands in the queue are
 *  completed.
 */
void	COCLDevice::blockUntilFinished()
{
	this->bBusy = true;
	clFlush(this->clQueue);
	clFinish(this->clQueue);
	/*
	if (clMarkerEvent != NULL) {
		clReleaseEvent(clMarkerEvent);
		clMarkerEvent = NULL;
	}
	*/
	this->bBusy = false;
}

/*
 *  Does this device fully support the required aspects of double precision
 */
bool COCLDevice::isDoubleCompatible()
{
	return (this->clDeviceDoubleFloatConfig &
		(CL_FP_FMA | CL_FP_ROUND_TO_NEAREST | CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_INF | CL_FP_INF_NAN | CL_FP_DENORM)) ==
		(CL_FP_FMA | CL_FP_ROUND_TO_NEAREST | CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_INF | CL_FP_INF_NAN | CL_FP_DENORM);
}

/*
 *  Release the event otherwise the 500 limit will be hit
 */
void CL_CALLBACK COCLDevice::defaultCallback(cl_event clEvent, cl_int iStatus, void* vData)
{
	//unsigned int uiDeviceNo = *(unsigned int*)vData; // Unused
	clReleaseEvent(clEvent);
}

/*
 * Flush the work to the device and use a marker we can wait on
 */
void COCLDevice::flushAndSetMarker()
{
	this->bBusy = true;
	clFlush(clQueue);
	return;

	#ifdef USE_SIMPLE_ARCH_OPENCL
	this->blockUntilFinished();
	return;
	#endif

	if (clMarkerEvent != NULL)
	{
		clReleaseEvent(clMarkerEvent);
	}

	// NOTE: OpenCL 1.2 uses clEnqueueMarkerWithWaitList() instead
	clEnqueueMarker(
		clQueue,
		&clMarkerEvent
	);

	clSetEventCallback(
		clMarkerEvent,
		CL_COMPLETE,
		COCLDevice::markerCallback,
		static_cast<void*>(&this->callBackData)
	);

	clFlush(clQueue);
}

/*
 *	Flush the work to the device
 */
void	COCLDevice::flush()
{
	clFlush(clQueue);
}

/*
 *  Mark this device as no longer being busy so we can queue some more work
 */
void CL_CALLBACK COCLDevice::markerCallback(cl_event clEvent, cl_int iStatus, void* vData)
{
	//unsigned int uiDeviceNo = *(unsigned int*) vData;
	model::CallBackData* callBackData = (model::CallBackData*)vData;
	unsigned int uiDeviceNo = *callBackData->DeviceNumber;
	clReleaseEvent(clEvent);

	COCLDevice* pDevice = callBackData->Executor->getDevice(uiDeviceNo);
	pDevice->markerCompletion();
}

/*
 *  Triggered once the marker callback has been, but this is a non-static function
 */
void COCLDevice::markerCompletion()
{
	this->clMarkerEvent = NULL;
	this->bBusy = false;
}

/*
*  Is this device currently busy?
*/
bool COCLDevice::isBusy()
{
	// To use the callback mechanism...
	return this->bBusy;

	if (clMarkerEvent == NULL)
		return false;

	cl_int iStatus;
	size_t szStatusSize;
	cl_int iQueryStatus = clGetEventInfo(
		clMarkerEvent,
		CL_EVENT_COMMAND_EXECUTION_STATUS,
		sizeof(cl_int),
		&iStatus,
		&szStatusSize
	);

	if (iQueryStatus != CL_SUCCESS)
		return true;

	model::log->logInfo("Exec status for device #" + toStringExact(uiDeviceNo)+" is " + toStringExact(iStatus));
	if (iStatus == CL_COMPLETE)
	{
		return false;
	}
	else {
		return true;
	}
}


/*
 *  Get a short name for the device
 */
std::string		COCLDevice::getDeviceShortName( void )
{
	std::string sName = "";

	if ( this->clDeviceType & CL_DEVICE_TYPE_CPU )			sName = "CPU ";
	if ( this->clDeviceType & CL_DEVICE_TYPE_GPU )			sName = "GPU ";
	if ( this->clDeviceType & CL_DEVICE_TYPE_ACCELERATOR )	sName = "APU ";

	sName += toStringExact( this->uiDeviceNo );

	return sName;
}

/*
 *  Fetch a struct with summary information
 */
void COCLDevice::getSummary(  COCLDevice::sDeviceSummary & pSummary )
{
	std::string sType = "Unknown";

	if (this->clDeviceType & CL_DEVICE_TYPE_CPU)			sType = "CPU";
	if (this->clDeviceType & CL_DEVICE_TYPE_GPU)			sType = "GPU";
	if (this->clDeviceType & CL_DEVICE_TYPE_ACCELERATOR)	sType = "APU";

	strncpy(pSummary.cDeviceName, this->clDeviceName, 99 );
	strncpy(pSummary.cDeviceType, sType.c_str(), 9 );
	pSummary.uiDeviceID = this->uiDeviceNo;
	pSummary.uiDeviceNumber = this->uiDeviceNo + 1;
}
