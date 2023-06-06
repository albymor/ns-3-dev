build_lib_example(
    NAME safety-example
    SOURCE_FILES safety-example.cc
    LIBRARIES_TO_LINK ${libsafety}
                      ${libwifi}
                      ${libapplications}
)

