set(GOOGLE_CLOUD_CPP_VERSION_MAJOR 0)
set(GOOGLE_CLOUD_CPP_VERSION_MINOR 7)
set(GOOGLE_CLOUD_CPP_VERSION_PATCH 0)
set(STORAGE_CLIENT_VERSION_MAJOR 1)
set(STORAGE_CLIENT_VERSION_MINOR 0)
set(STORAGE_CLIENT_VERSION_PATCH 0)

set(GCP_SOURCES_DIR ${CMAKE_BINARY_DIR}/google-cloud-cpp-0.9.0)
set(GCP_URL "http://orthanc.osimis.io/ThirdPartyDownloads/dicom-web/google-cloud-cpp-0.9.0.tar.gz")
set(GCP_MD5 "b7546b6b11d23dad6cf0c77ddf6c567b")

if (IS_DIRECTORY "${GCP_SOURCES_DIR}")
  set(FirstRun OFF)
else()
  set(FirstRun ON)
endif()

DownloadPackage(${GCP_MD5} ${GCP_URL} "${GCP_SOURCES_DIR}")

execute_process(
  COMMAND ${PATCH_EXECUTABLE} -p0 -N -i
  ${CMAKE_CURRENT_LIST_DIR}/../Patches/google-cloud-cpp-0.9.0.patch
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  RESULT_VARIABLE Failure
  )

if (FirstRun AND Failure)
  message(FATAL_ERROR "Error while patching a file")
endif()

configure_file(
  ${GCP_SOURCES_DIR}/google/cloud/internal/version_info.h.in
  ${AUTOGENERATED_DIR}/google/cloud/internal/version_info.h
  )

configure_file(
  ${GCP_SOURCES_DIR}/google/cloud/storage/version_info.h.in
  ${AUTOGENERATED_DIR}/google/cloud/storage/version_info.h
  )

configure_file(
  ${GCP_SOURCES_DIR}/google/cloud/internal/build_info.cc.in
  ${AUTOGENERATED_DIR}/google/cloud/internal/build_info.cc
  )

include_directories(
  ${GCP_SOURCES_DIR}
  )

set(GCP_DIRECTORIES
  ${GCP_SOURCES_DIR}/google/cloud
  ${GCP_SOURCES_DIR}/google/cloud/internal
  ${GCP_SOURCES_DIR}/google/cloud/storage
  ${GCP_SOURCES_DIR}/google/cloud/storage/internal
  ${GCP_SOURCES_DIR}/google/cloud/storage/oauth2
  )

set(GCP_SOURCES
  ${AUTOGENERATED_DIR}/google/cloud/internal/build_info.cc
  )

foreach(d ${GCP_DIRECTORIES})
  set(TMP)
  aux_source_directory(${d} TMP)

  foreach(i ${TMP})
    if(NOT i MATCHES ".*_test.cc")
      list(APPEND GCP_SOURCES ${i})
    endif()
  endforeach()
endforeach()


#list(REMOVE_ITEM GCP_SOURCES
#  ${GCP_SOURCES_DIR}/google/cloud/storage/internal/openssl_util.cc
#  )
