/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

#ifndef HIPIMS_DOMAIN_CEXECUTORCONTROL_H_
#define HIPIMS_DOMAIN_CEXECUTORCONTROL_H_

/*
 *  EXECUTOR CONTROL CLASS
 *  CExecutorControl
 *
 *  Controls the model execution
 */
class CExecutorControl
{

	public:

		CExecutorControl( void );										
		virtual ~CExecutorControl( void );								

		// Public functions
		bool						isReady( void );					// Is the executor ready?
		void						setDeviceFilter( unsigned int );	// Filter to specific types of device
		unsigned int				getDeviceFilter();					// Fetch back the current device filter
		virtual bool				createDevices(void) = 0;			// Creates new classes for each device

		// Static functions
		static CExecutorControl*	createExecutor( unsigned char, CModel*);	// Create a new executor of the specified type
		
	protected:

		// Protected functions
		void					setState( unsigned int );				// Set the ready state
		unsigned int			deviceFilter;							// Device filter active for this executor

	private:

		// Private variables
		unsigned int			state;									// Ready state value

};

#endif
