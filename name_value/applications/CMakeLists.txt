SET( dir ${SOURCE_CODE_BASE_DIR}/name_value/applications )
FILE( GLOB source   ${dir}/*.cpp)
FILE( GLOB includes ${dir}/*.h)
SOURCE_GROUP( ${TARGET_NAME} FILES ${source} ${includes} )
ADD_EXECUTABLE( name-value-convert ${dir}/name-value-convert.cpp )
ADD_EXECUTABLE( name-value-get ${dir}/name-value-get.cpp )
ADD_EXECUTABLE( name-value-eval-preparse ${dir}/name-value-eval-preparse.cpp )
ADD_EXECUTABLE( name-value-permute ${dir}/name-value-permute.cpp )

TARGET_LINK_LIBRARIES( name-value-convert comma_application comma_string comma_xpath comma_name_value )
TARGET_LINK_LIBRARIES( name-value-get comma_application comma_string comma_xpath comma_name_value )
TARGET_LINK_LIBRARIES( name-value-eval-preparse comma_application comma_name_value )
TARGET_LINK_LIBRARIES( name-value-permute comma_application comma_string comma_xpath comma_name_value )

INSTALL( TARGETS name-value-convert name-value-get name-value-eval-preparse name-value-permute
         RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR}
         COMPONENT Runtime )

install( PROGRAMS name-value-eval name-value-calc name-value-apply DESTINATION ${comma_INSTALL_BIN_DIR} )
         
add_executable( name-value-from-csv ${dir}/name-value-from-csv.cpp )
target_link_libraries( name-value-from-csv comma_application comma_string )
install( TARGETS name-value-from-csv RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR} COMPONENT Runtime )

add_executable( name-value-to-csv ${dir}/name-value-to-csv.cpp )
target_link_libraries( name-value-to-csv comma_application comma_string )
install( TARGETS name-value-to-csv RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR} COMPONENT Runtime )
