check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources    
        #test/safety-examples-test-suite.cc
        )
endif()    

build_lib(
    LIBNAME safety
    SOURCE_FILES model/failsafeManager.cc
                 model/failsafeTag.cc
                 model/seqnumTag.cc
                 model/IEEE-loss-model.cc
                 model/safety-udp-echo-client.cc
                 model/safety-udp-echo-helper.cc
                 model/safety-udp-echo-server.cc 
    HEADER_FILES model/failsafeManager.h
                 model/failsafeTag.h
                 model/seqnumTag.h
                 model/IEEE-loss-model.h
                 model/safety-udp-echo-client.h
                 model/safety-udp-echo-helper.h
                 model/safety-udp-echo-server.h 
    LIBRARIES_TO_LINK ${libcore}
    TEST_SOURCES test/safety-test-suite.cc
                 ${examples_as_tests_sources}
)
    
