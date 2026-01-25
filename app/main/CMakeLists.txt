
# Current File Path
# ${CAMAKE_SOURCE_DIR}/app/main/CMakeLists.txt

message(STATUS "app/main/CMakeLists.txt")

set(demo_uart
    "${CMAKE_SOURCE_DIR}/app/demo/uart/demo_app_uart_sync.c"
    "${CMAKE_SOURCE_DIR}/app/demo/uart/demo_app_uart_async.c"
)

# set(demo_nal
# "${CMAKE_SOURCE_DIR}/app/demo/nal/demo_nal_sync_tcp.c"
# "${CMAKE_SOURCE_DIR}/app/demo/nal/demo_nal_sync_tls.c"
# )
set(demo_wifi

    "${CMAKE_SOURCE_DIR}/app/demo/wifi/demo_wifi_sta_ap.c"
)

set(demo_conn_mgnr

    "${CMAKE_SOURCE_DIR}/app/conn_mgnr/conn_mgnr.c"
)

# set(ridecast_app
# "${CMAKE_SOURCE_DIR}/app/demo/wifi/demo_wifi_sta_ap.c"
# )
idf_component_register(
    SRCS "app_main.c"
    "${CMAKE_SOURCE_DIR}/external/os/cmsis_esp/Source/cmsis_os2.c"

    # "${CMAKE_SOURCE_DIR}/app/demo_config/app_bsp_config.c"
    ${demo_uart}
    ${demo_wifi}
    ${demo_conn_mgnr}

    INCLUDE_DIRS "."
    "${CMAKE_SOURCE_DIR}/external/os/cmsis_esp/Include"
    "${CMAKE_SOURCE_DIR}/app/demo_config"

    PRIV_REQUIRES
    freertos
    cmsis_esp
    system
    mbedtls
    lwip
    driver
    esp_netif
    esp_wifi
    esp_event
    log
    mbedtls
    lwip
    esp-tls
    esp_common
    bsp
    nal
    cli
    nvs_flash
    backoffAlgorithmWrapper
)
