/*

Cluster                           RideCast
   |                                 |
   |---- WIFI_SET_CONFIG ----------->|
   |<--- ACK ------------------------|
   |                                 |
   |---- BLE_SET_CONFIG ------------>|
   |<--- ACK ------------------------|
   |                                 |
   |---- BT_CLASSIC_SET_CONFIG ----->|
   |<--- ACK ------------------------|
   |                                 |
   |---- START_ALL ----------------->|
   |<--- ACK ------------------------|
   |                                 |
   |<--- WIFI_STATUS: READY ---------|
   |<--- BLE_STATUS: READY ----------|
   |<--- BT_CLASSIC_STATUS: READY ---|


*/