build_lib_example(
    NAME tsn-wifi
    SOURCE_FILES tsn-wifi.cc
    LIBRARIES_TO_LINK ${libsafety}
                      ${libwifi}
                      ${libapplications}
                      ${libtsn}
)

