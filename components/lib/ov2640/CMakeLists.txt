# get IDF version for comparison
set(idf_version "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}")

# set conversion sources
set(COMPONENT_SRCS
  ov2640/conversions/yuv.c
  ov2640/conversions/to_jpg.cpp
  ov2640/conversions/to_bmp.c
  ov2640/conversions/jpge.cpp
  ov2640/conversions/esp_jpg_decode.c
  )

set(COMPONENT_PRIV_INCLUDEDIRS
  ov2640/conversions/private_include
  )

set(COMPONENT_ADD_INCLUDEDIRS
  ov2640/driver/include
  ov2640/conversions/include
  )

set(COMPONENT_REQUIRES driver)

# set driver sources only for supported platforms
if(IDF_TARGET STREQUAL "esp32" OR IDF_TARGET STREQUAL "esp32s2" OR IDF_TARGET STREQUAL "esp32s3")
  list(APPEND COMPONENT_SRCS
    ov2640/driver/esp_camera.c
    ov2640/driver/cam_hal.c
    ov2640/driver/sccb.c
    ov2640/driver/sensor.c
    ov2640/sensors/ov2640.c
    ov2640/sensors/ov3660.c
    ov2640/sensors/ov5640.c
    ov2640/sensors/ov7725.c
    ov2640/sensors/ov7670.c
    ov2640/sensors/nt99141.c
    ov2640/sensors/gc0308.c
    ov2640/sensors/gc2145.c
    ov2640/sensors/gc032a.c
    ov2640/sensors/bf3005.c
    ov2640/sensors/bf20a6.c
    ov2640/sensors/sc030iot.c
    )

  list(APPEND COMPONENT_PRIV_INCLUDEDIRS
    ov2640/driver/private_include
    ov2640/sensors/private_include
    ov2640/target/private_include
    )

  if(IDF_TARGET STREQUAL "esp32")
    list(APPEND COMPONENT_SRCS
      ov2640/target/xclk.c
      ov2640/target/esp32/ll_cam.c
      )
  endif()

  if(IDF_TARGET STREQUAL "esp32s2")
    list(APPEND COMPONENT_SRCS
      ov2640/target/xclk.c
      ov2640/target/esp32s2/ll_cam.c
      ov2640/target/esp32s2/tjpgd.c
      )

    list(APPEND COMPONENT_PRIV_INCLUDEDIRS
      ov2640/target/esp32s2/private_include
      )
  endif()

  if(IDF_TARGET STREQUAL "esp32s3")
    list(APPEND COMPONENT_SRCS
      ov2640/target/esp32s3/ll_cam.c
      )
  endif()

  set(COMPONENT_PRIV_REQUIRES freertos nvs_flash)

  set(min_version_for_esp_timer "4.2")
  if (idf_version VERSION_GREATER_EQUAL min_version_for_esp_timer)
    list(APPEND COMPONENT_PRIV_REQUIRES esp_timer)
  endif()

endif()

register_component()
