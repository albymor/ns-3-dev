build_lib_example(
    NAME tsn-safety
    SOURCE_FILES tsn-safety.cc
    LIBRARIES_TO_LINK ${libsafety}
                      ${libwifi}
                      ${libapplications}
                      ${libtsn}
)

