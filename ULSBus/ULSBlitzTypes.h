#ifndef ULSBLITZTYPES_H
#define ULSBLITZTYPES_H
#include <inttypes.h>

#define ULS_BLITZ_MSG_ID_XCG1_RX_RAW_MEASURMENT 0x0010
typedef struct __attribute__((packed)) {
  // Status :
  //  BIT                             Description
  //  0   ULS_STATUS_RX_CARRIER_OK  - get IR signal
  //  1   ULS_STATUS_RX_SQ_OK       - signal quality OK
  //  2   ULS_STATUS_RX_POS_OK      - Position estimated
  //  3   ULS_STATUS_RX_VEL_OK      - Velocity estimated
  //  4   ULS_STATUS_RX_GU_IMU_OK   - Ground Unit IMU ok
  //  5   ULS_STATUS_RX_MRX_OK      - MRX message received (secondary receiver )
  //  6   ULS_STATUS_RX_MRXDATA_OK  - MRX Yaw and DIstance Calculated - OK

  uint8_t status;  // bit[0] = (1=ok)(0=error)
  uint8_t d;       // distance in cm
  // angular information proportional [-30000 = -30/ +30000 = +30]
  int16_t aX;
  int16_t aY;
  // sensor offset information  cm.
  int8_t oF;
  int8_t oR;
} __uls_blitz_xcg1_rx_raw_measurmet;

#define ULS_BLITZ_MSG_ID_RX_RAW_MEASURMENT 0x0011
typedef struct __attribute__((packed)) {
  // Status :
  //  BIT                             Description
  //  0   ULS_STATUS_RX_CARRIER_OK  - get IR signal
  //  1   ULS_STATUS_RX_SQ_OK       - signal quality OK
  //  2   ULS_STATUS_RX_POS_OK      - Position estimated
  //  3   ULS_STATUS_RX_VEL_OK      - Velocity estimated
  //  4   ULS_STATUS_RX_GU_IMU_OK   - Ground Unit IMU ok
  //  5   ULS_STATUS_RX_MRX_OK      - MRX message received (secondary receiver )
  //  6   ULS_STATUS_RX_MRXDATA_OK  - MRX Yaw and DIstance Calculated - OK

  uint8_t status;
  uint8_t counter;
  // angular information proportional [-180 ... +180 degrees]
  // deg = (value_int16 * 180.f)/32767.f
  int16_t aX;
  int16_t aY;

  uint16_t distance;

} __uls_blitz_msg_rx_raw_measurmet;

#define ULS_BLITZ_MSG_ID_RX_POS_IN_TX_FRAME 0x0012
typedef struct __attribute__((packed)) {
  // Status :
  //  BIT                             Description
  //  0   ULS_STATUS_RX_CARRIER_OK  - get IR signal
  //  1   ULS_STATUS_RX_SQ_OK       - signal quality OK
  //  2   ULS_STATUS_RX_POS_OK      - Position estimated
  //  3   ULS_STATUS_RX_VEL_OK      - Velocity estimated
  //  4   ULS_STATUS_RX_GU_IMU_OK   - Ground Unit IMU ok
  //  5   ULS_STATUS_RX_MRX_OK      - MRX message received (secondary receiver )
  //  6   ULS_STATUS_RX_MRXDATA_OK  - MRX Yaw and DIstance Calculated - OK

  uint8_t status;  // bit[0] = (1=ok)(0=error)
  uint8_t counter;
  // Position vector in transmitter frame in cm.
  int16_t posX;
  int16_t posY;
  int16_t posZ;

} __uls_blitz_msg_rx_pos_in_tx_frame;

// Receiver  MRX Information.
#define ULS_BLITZ_MSG_ID_RX_MRX 0x0013
typedef struct __attribute__((packed)) {
  // Status :
  //  BIT                             Description
  //  0   ULS_STATUS_RX_CARRIER_OK  - get IR signal
  //  1   ULS_STATUS_RX_SQ_OK       - signal quality OK
  //  2   ULS_STATUS_RX_POS_OK      - Position estimated
  //  3   ULS_STATUS_RX_VEL_OK      - Velocity estimated
  //  4   ULS_STATUS_RX_GU_IMU_OK   - Ground Unit IMU ok
  //  5   ULS_STATUS_RX_MRX_OK      - MRX message received (secondary receiver )
  //  6   ULS_STATUS_RX_MRXDATA_OK  - MRX Yaw and DIstance Calculated - OK
  uint8_t status;  // bit[0] = (1=ok)(0=error)
  uint8_t counter;
  // angular information proportional [-180 ... +180 degrees]
  int16_t yaw;
  // value in [cm]
  uint16_t distance;
} __uls_blitz_msg_rx_mrx;

// Receiver  MRX Information.
#define ULS_BLITZ_MSG_ID_RX_DACS 0x0014
typedef struct __attribute__((packed)) {
  // Status :
  //  BIT                             Description
  //  0   ULS_STATUS_RX_CARRIER_OK  - get IR signal
  //  1   ULS_STATUS_RX_SQ_OK       - signal quality OK
  //  2   ULS_STATUS_RX_POS_OK      - Position estimated
  //  3   ULS_STATUS_RX_VEL_OK      - Velocity estimated
  //  4   ULS_STATUS_RX_GU_IMU_OK   - Ground Unit IMU ok
  //  5   ULS_STATUS_RX_MRX_OK      - MRX message received (secondary receiver )
  //  6   ULS_STATUS_RX_MRXDATA_OK  - MRX Yaw and DIstance Calculated - OK

  uint8_t status;  // bit[0] = (1=ok)(0=error)
  uint8_t counter;
  // angular information proportional [-180 ... +180 degrees]
  int16_t yaw;
  uint16_t distance;
  // angular information in deg +/- 32768 deg
  int16_t yawContinuous;

} __uls_blitz_msg_rx_dacs;

#define ULS_BLITZ_MSG_ID_GIMU_MEASUREMENT 0x0020
typedef struct __attribute__((packed)) {
  // Status :
  //  BIT                             Description
  //  0   ULS_STATUS_RX_CARRIER_OK  - get IR signal
  //  1   ULS_STATUS_RX_SQ_OK       - signal quality OK
  //  2   ULS_STATUS_RX_POS_OK      - Position estimated
  //  3   ULS_STATUS_RX_VEL_OK      - Velocity estimated
  //  4   ULS_STATUS_RX_GU_IMU_OK   - Ground Unit IMU ok
  //  5   ULS_STATUS_RX_MRX_OK      - MRX message received (secondary receiver )
  //  6   ULS_STATUS_RX_MRXDATA_OK  - MRX Yaw and DIstance Calculated - OK
  uint8_t status;  // bit[0] = (1=ok)(0=error)
  uint8_t counter;
  // angular information proportional [-180 ... +180 degrees]
  // deg = (value_int16 * 180.f)/32767.f
  int16_t roll;
  int16_t pitch;
  int16_t yaw;

} __uls_blitz_msg_gimu_data;

#endif  // ULSBLITZTYPES_H
