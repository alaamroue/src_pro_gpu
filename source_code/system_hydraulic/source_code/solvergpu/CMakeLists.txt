set(UI_FILES
)

set(H_FILES
    source_code/CBenchmark.h
    source_code/CDomain.h
    source_code/CDomainBase.h
    source_code/CDomainCartesian.h
    source_code/CDomainLink.h
    source_code/CDomainManager.h
    source_code/CDomainRemote.h
    source_code/CExecutorControl.h
    source_code/CExecutorControlOpenCL.h
    source_code/CLog.h
    source_code/CModel.h
    source_code/CMultiGpuManager.h
    source_code/COCLBuffer.h
    source_code/COCLDevice.h
    source_code/COCLKernel.h
    source_code/COCLProgram.h
    source_code/common.h
    source_code/CProfiler.h
    source_code/CScheme.h
    source_code/CSchemeGodunov.h
    source_code/CSchemeInertial.h
    source_code/CSchemeMUSCLHancock.h
    source_code/CSchemePromaides.h
    source_code/gpudemo.h
    source_code/resource.h
    source_code/util.h
)

set(CPP_FILES           
    source_code/CBenchmark.cpp
    source_code/CDomain.cpp
    source_code/CDomainBase.cpp
    source_code/CDomainCartesian.cpp
    source_code/CDomainLink.cpp
    source_code/CDomainManager.cpp
    source_code/CDomainRemote.cpp
    source_code/CExecutorControl.cpp
    source_code/CExecutorControlOpenCL.cpp
    source_code/CLog.cpp
    source_code/CModel.cpp
    source_code/CMultiGpuManager.cpp
    source_code/COCLBuffer.cpp
    source_code/COCLDevice.cpp
    source_code/COCLKernel.cpp
    source_code/COCLProgram.cpp
    source_code/CProfiler.cpp
    source_code/CScheme.cpp
    source_code/CSchemeGodunov.cpp
    source_code/CSchemeInertial.cpp
    source_code/CSchemeMUSCLHancock.cpp
    source_code/CSchemePromaides.cpp
    source_code/gpudemo.cpp
    source_code/util.cpp
    source_code/windows_platform.cpp
)
## THIS NEEDS TO BE INCULDED IN THE EXECUTABLE, "NOT" HERE
## AFAIK you can't add resources to a shared library 
set(RCFiles
    CLCode.rc
)

#qt5_wrap_ui(UI_FILES ${UI_FILES})

add_library(solvergpu ${UI_FILES} ${H_FILES} ${CPP_FILES})

# To allow correct correct recovery of exeptions through the stack
if(MSVC)
    target_compile_options(solvergpu PRIVATE /EHsc)
endif()

find_package(OpenCL REQUIRED)

target_link_libraries(solvergpu OpenCL::OpenCL)


target_include_directories(solvergpu
    PUBLIC
    	/
        source_code/
        ${OpenCL_INCLUDE_DIRS}
)
message("Including OpenCL libraries to solvergpu lib, path is:" ${OpenCL_INCLUDE_DIRS})