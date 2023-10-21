/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

#ifndef HIPIMS_CMODEL_H_
#define HIPIMS_CMODEL_H_

#include "CL/opencl.h"
#include "CBenchmark.h"
#include <vector>

// Some classes we need to know about...
class CExecutorControl;
class CExecutorControlOpenCL;
class CDomainManager;
class CScheme;
class CLog;
class CProfiler;
class CMPIManager;


/*
 *  APPLICATION CLASS
 *  CModel
 *
 *  Is a singleton class in reality, but need not be enforced.
 */
class CModel
{
	public:

		// Public functions
		CModel(void);															// Constructor
		~CModel(void);															// Destructor

		bool					setExecutor(CExecutorControl*);					// Sets the type of executor to use for the model
		CExecutorControlOpenCL*	getExecutor(void);								// Gets the executor object currently in use
		CDomainManager*			getDomainSet(void);								// Gets the domain set
		CMPIManager*			getMPIManager(void);							// Gets the MPI manager
		void					setSelectedDevice(unsigned int);
		unsigned int			getSelectedDevice();

		bool					runModel(void);									// Execute the model
		void					runModelPrepare(void);							// Prepare for model run
		void					runModelPrepareDomains(void);					// Prepare domains and domain links
		//void					runModelMain(void);								// Main model run loop
		void					runModelDomainAssess( bool* );			// Assess domain states
		void					runModelDomainExchange(void);					// Exchange domain data
		void					runModelUpdateTarget(double);					// Calculate a new target time
		void					runModelSync(void);								// Synchronise domain and timestep data
		void					runModelOutputs(void);							// Process outputs
		void					runModelMPI(void);								// Process MPI queue etc.
		void					runModelSchedule( CBenchmark::sPerformanceMetrics *, bool * );	// Schedule work
		void					runModelUI( CBenchmark::sPerformanceMetrics * );// Update progress data etc.
		void					runModelRollback(void);							// Rollback simulation
		void					runModelBlockGlobal(void);						// Block all domains until all are done
		void					runModelBlockNode(void);						// Block further processing on this node only
		void					runModelCleanup(void);							// Clean up after a simulation completes/aborts

		void					logDetails();									// Spit some info out to the log
		double					getSimulationLength();							// Get total length of simulation
		void					setSimulationLength( double );					// Set total length of simulation
		double					getOutputFrequency();							// Get the output frequency
		void					setOutputFrequency( double );					// Set the output frequency
		void					setFloatPrecision( unsigned char );				// Set floating point precision
		unsigned char			getFloatPrecision();							// Get floating point precision
		void					setName( std::string );							// Sets the name
		void					setDescription( std::string );					// Sets the description
		void					logProgress( CBenchmark::sPerformanceMetrics* );// Write the progress bar etc.
		static void CL_CALLBACK	visualiserCallback( cl_event, cl_int, void * );	// Callback event used when memory reads complete, for visualisation updates
		void					runNext(const double);
		double*					getBufferOpt();

		// Public variables
		void					setLogger(CLog*);								// Sets the logger class 
		CLog*					log;											// Handle for the log singular class
		void					setUIStatus(bool);								// Turns on/off the UI

		void					setProfiler(CProfiler*);						// 
		CProfiler* profiler;													// 
	private:

		// Private functions
		void					visualiserUpdate();								// Update 3D stuff 

		// Private variables
		CExecutorControlOpenCL*	execController;									// Handle for the executor controlling class
		CDomainManager*			domains;										// Handle for the domain management class
		CMPIManager*			mpiManager;										// Handle for the MPI manager class
		unsigned int			selectedDevice;
		std::string				sModelName;										// Short name for the model
		std::string				sModelDescription;								// Short description of the model
		bool					bDoublePrecision;								// Double precision enabled?
		double					dSimulationTime;								// Total length of simulations
		double					dCurrentTime;									// Current simulation time
		double					dVisualisationTime;								// Current visualisation time
		double					dProcessingTime;								// Total processing time
		double					dOutputFrequency;								// Frequency of outputs
		double					dLastSyncTime;									//
		double					dLastOutputTime;								//
		double					dLastProgressUpdate;							//
		double					dTargetTime;									// 
		double					dEarliestTime;									//
		double					dGlobalTimestep;								//
		unsigned long			ulRealTimeStart;
		bool					bRollbackRequired;								// 
		bool					bAllIdle;										//
		bool					bWaitOnLinks;									//
		bool					bSynchronised;									//
		unsigned char			ucFloatSize;									// Size of single/double precision floats used
		cursorCoords			pProgressCoords;								// Buffer coords of the progress output
		bool					showProgess;									// Show Progess UI

};

#endif
