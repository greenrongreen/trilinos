
INCLUDE(Parse_Variable_Arguments)


#
# Function that copies a list of source files
# to a list of destination files between two
# base directories.
#

FUNCTION(COPY_FILES_TO_BINARY_DIR TARGET_NAME)

  #
  # A) Parse input arguments
  #

  PARSE_ARGUMENTS(
    #prefix
    PARSE
    #lists
    "SOURCE_DIR;SOURCE_FILES;SOURCE_PREFIX;DEST_DIR;DEST_FILES"
    #options
    ""
    ${ARGN}
    )

  IF (NOT PARSE_SOURCE_DIR)
    SET(PARSE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  ENDIF()

  IF (NOT PARSE_DEST_DIR)
    SET(PARSE_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR})
  ENDIF()

  #
  # B) Validate arguments
  #

  LIST(LENGTH PARSE_SOURCE_DIR PARSE_SOURCE_DIR_LEN)
  LIST(LENGTH PARSE_SOURCE_FILES PARSE_SOURCE_FILES_LEN)
  LIST(LENGTH PARSE_SOURCE_PREFIX PARSE_SOURCE_PREFIX_LEN)
  LIST(LENGTH PARSE_DEST_DIR PARSE_DEST_DIR_LEN)
  LIST(LENGTH PARSE_DEST_FILES PARSE_DEST_FILES_LEN)

  IF (PARSE_SOURCE_DIR_LEN GREATER 1)
    MESSAGE(SEND_ERROR "Error, there can only be 0 or one SOURCE_DIR arguments!")
  ENDIF()

  IF (PARSE_DEST_DIR_LEN GREATER 1)
    MESSAGE(SEND_ERROR "Error, there can only be 0 or one DEST_DIR arguments!")
  ENDIF()

  IF (PARSE_DEST_FILES_LEN EQUAL 0)
    MESSAGE(SEND_ERROR "Error, there are no files listed!")
  ENDIF()

  IF (PARSE_SOURCE_PREFIX)

    IF (NOT PARSE_SOURCE_PREFIX_LEN EQUAL 1)
      MESSAGE(SEND_ERROR "Error, If SOURCE_PREFIX can only take one argument!")
    ENDIF()

    IF (NOT PARSE_SOURCE_FILES_LEN EQUAL 0)
      MESSAGE(SEND_ERROR "Error, If SOURCE_PREFIX is specified then SOURCE_FILES must not be!")
    ENDIF()

  ELSE()

    IF (NOT PARSE_SOURCE_FILES_LEN EQUAL ${PARSE_DEST_FILES_LEN})
      MESSAGE(SEND_ERROR "Error, there are not the same number of source files ${PARSE_SOURCE_FILES_LEN} and dest files ${PARSE_DEST_FILES_LEN}!")
    ENDIF()

  ENDIF()

  #
  # C) Build the list of command and dependencies
  #

  MATH(EXPR FILES_IDX_END "${PARSE_DEST_FILES_LEN}-1")

  FOREACH(FILE_IDX RANGE ${FILES_IDX_END})

    LIST(GET PARSE_DEST_FILES ${FILE_IDX} DEST_FILE)
    SET(DEST_FILE_FULL "${PARSE_DEST_DIR}/${DEST_FILE}")

    IF (PARSE_SOURCE_PREFIX)
      SET(SOURCE_FILE_FULL "${PARSE_SOURCE_DIR}/${PARSE_SOURCE_PREFIX}${DEST_FILE}")
    ELSE()
      LIST(GET PARSE_SOURCE_FILES ${FILE_IDX} SOURCE_FILE)
      SET(SOURCE_FILE_FULL "${PARSE_SOURCE_DIR}/${SOURCE_FILE}")
    ENDIF()
  
    #PRINT_VAR(SOURCE_FILE_FULL)
    #PRINT_VAR(DEST_FILE_FULL)

    ADD_CUSTOM_COMMAND(
      OUTPUT ${DEST_FILE_FULL}
      DEPENDS ${SOURCE_FILE_FULL}
      COMMAND ${CMAKE_COMMAND} ARGS -E copy ${SOURCE_FILE_FULL} ${DEST_FILE_FULL}
      )

    IF (DEFINED DEST_FILES_LIST)
      SET(DEST_FILES_LIST ${DEST_FILES_LIST} ${DEST_FILE_FULL})
    ELSE()
      SET(DEST_FILES_LIST ${DEST_FILE_FULL})
    ENDIF()
    

  ENDFOREACH()

  #PRINT_VAR(DEST_FILES_LIST)

  ADD_CUSTOM_TARGET( ${TARGET_NAME} ALL
    DEPENDS ${DEST_FILES_LIST}
    )

ENDFUNCTION()
