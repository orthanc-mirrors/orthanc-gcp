set(CRC32C_SOURCES_DIR ${CMAKE_BINARY_DIR}/crc32c-1.0.6)
set(CRC32C_URL "http://orthanc.osimis.io/ThirdPartyDownloads/dicom-web/crc32c-1.0.6.tar.gz")
set(CRC32C_MD5 "e7eaad378aeded322d27a35b0011d626")
DownloadPackage(${CRC32C_MD5} ${CRC32C_URL} "${CRC32C_SOURCES_DIR}")

configure_file(
  ${CRC32C_SOURCES_DIR}/src/crc32c_config.h.in
  ${AUTOGENERATED_DIR}/crc32c/crc32c_config.h
  )

include_directories(
  ${CRC32C_SOURCES_DIR}/include
  )

set(CRC32C_SOURCES
  ${CRC32C_SOURCES_DIR}/src/crc32c.cc
  ${CRC32C_SOURCES_DIR}/src/crc32c_arm64.cc
  ${CRC32C_SOURCES_DIR}/src/crc32c_portable.cc
  ${CRC32C_SOURCES_DIR}/src/crc32c_sse42.cc
  )
