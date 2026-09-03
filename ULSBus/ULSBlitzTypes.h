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
  uint8_t d;       // distance in 10 cm
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

// Override IMU data on ground unit.
// This data works like compass override mode and allow to set
// roll, pitch and yaw angles on ground unit.
// This data are transmitted to receiver over IR channel and
// Used to calculate orientation of receiver in earth frame.

#define ULS_BLITZ_MSG_ID_XX_TXC_IMU_OVERRIDE 0x0030
typedef struct __attribute__((packed)) {
  // Control :
  //  BIT                             Description
  //  0   SET Roll                 - Set Roll angle override
  //  1   SET Pitch                 - Set Pitch angle override
  //  2   SET Yaw                   - Set Yaw angle override
  //  3-7 reserved
  uint8_t control;  //
  uint8_t counter;  // packets counter
  // angular information proportional [-180 ... +180 degrees]
  // deg = (value_int16 * 180.f)/32767.f
  int16_t roll;
  int16_t pitch;
  int16_t yaw;

} __uls_blitz_msg_xx_txc_imu_override;

// Override MAG data on ground unit.
// This data works like compass override mode and allow to set
// external magnetometer values on ground unit.
// This data are transmitted to receiver over IR channel and
// Used to calculate orientation of receiver in earth frame.

#define ULS_BLITZ_MSG_ID_XX_TXC_MAG_OVERRIDE 0x0031
typedef struct __attribute__((packed)) {
  // Control :
  //  BIT                             Description
  //  0   SET override                 - Set Mag values override
  //  1-7 reserved
  uint8_t control;  //
  uint8_t counter;  // packets counter
  // magnetic field information proportional
  // mag_value = (value_int16 * 1.f)/32767.f
  int16_t mx;
  int16_t my;
  int16_t mz;

} __uls_blitz_msg_xx_txc_mag_override;

// ---------------------------------------------------------------------------
// ULS-XX-EIGC-G3 : External IMU / GNSS / Compass module.
//
// Two of these modules form a GNSS compass. Each one broadcasts its own antenna
// fix and its offset from the platform centre; the peer differences the two
// global positions to derive platform yaw. Every payload below is 8 bytes, so
// each message is a single CAN frame and travels over ULSDeconet unchanged.
//
// A nav set (EPOCH, LAT, LON, VEL) is tied together by the shared "counter"
// field: the receiver must only difference positions carrying the SAME counter,
// otherwise the two fixes are from different epochs and the baseline smears
// while the platform moves. EPOCH carries the GNSS time of week that the
// counter maps to.
// ---------------------------------------------------------------------------

// Status bits shared by the EIGC nav messages:
//  BIT   Description
//  0     ULS_STATUS_EIGC_FIX_OK        - GNSS fix usable
//  1     ULS_STATUS_EIGC_FIX_3D        - 3D fix (not 2D / dead reckoning)
//  2     ULS_STATUS_EIGC_DIFF_SOLN     - differential corrections applied
//  3     ULS_STATUS_EIGC_HEADING_OK    - magnetic heading valid
//  4     ULS_STATUS_EIGC_OFFSET_OK     - platform centre offset configured
//  5     ULS_STATUS_EIGC_IS_PRIMARY    - sender is the primary of the pair
//  6     ULS_STATUS_EIGC_IMU_OK        - IMU and both compasses healthy
//  7     ULS_STATUS_EIGC_TIME_OK       - GNSS time valid and fully resolved
#define ULS_STATUS_EIGC_FIX_OK 0x01
#define ULS_STATUS_EIGC_FIX_3D 0x02
#define ULS_STATUS_EIGC_DIFF_SOLN 0x04
#define ULS_STATUS_EIGC_HEADING_OK 0x08
#define ULS_STATUS_EIGC_OFFSET_OK 0x10
#define ULS_STATUS_EIGC_IS_PRIMARY 0x20
#define ULS_STATUS_EIGC_IMU_OK 0x40
#define ULS_STATUS_EIGC_TIME_OK 0x80

// Epoch tag for one navigation set. Sent first, then LAT / LON / VEL with the
// same counter value.
#define ULS_BLITZ_MSG_ID_EIGC_NAV_EPOCH 0x0040
typedef struct __attribute__((packed)) {
  uint8_t status;   // ULS_STATUS_EIGC_*
  uint8_t counter;  // groups this nav set, increments once per epoch
  // GPS time of week of the fix this set describes.
  uint32_t iTOW;  // ms
  uint8_t numSV;  // satellites used
  // Horizontal accuracy estimate, saturating at 25.5 m. The peer weighs the
  // baseline solution by this.
  uint8_t hAcc;  // dm
} __uls_blitz_msg_eigc_nav_epoch;

#define ULS_BLITZ_MSG_ID_EIGC_NAV_LAT 0x0041
typedef struct __attribute__((packed)) {
  uint8_t status;
  uint8_t counter;
  int32_t lat;     // 1e-7 deg
  int16_t height;  // dm above mean sea level, +/-3276 m
} __uls_blitz_msg_eigc_nav_lat;

#define ULS_BLITZ_MSG_ID_EIGC_NAV_LON 0x0042
typedef struct __attribute__((packed)) {
  uint8_t status;
  uint8_t counter;
  int32_t lon;  // 1e-7 deg
  // Magnetic heading of the sender, the coarse reference that resolves which
  // end of the baseline is forward.
  // deg = (value_int16 * 180.f)/32767.f
  int16_t heading;
} __uls_blitz_msg_eigc_nav_lon;

#define ULS_BLITZ_MSG_ID_EIGC_NAV_VEL 0x0043
typedef struct __attribute__((packed)) {
  uint8_t status;
  uint8_t counter;
  // Velocity of the sender antenna in NED frame.
  int16_t velN;  // cm/s
  int16_t velE;  // cm/s
  int16_t velD;  // cm/s
} __uls_blitz_msg_eigc_nav_vel;

// Offset of the sender antenna from the platform centre, platform body frame.
// Static configuration, so it can go out at a much lower rate than the nav set;
// it is broadcast so neither module has to be told the other mounting by hand.
#define ULS_BLITZ_MSG_ID_EIGC_NAV_OFFSET 0x0044
typedef struct __attribute__((packed)) {
  uint8_t status;
  uint8_t counter;
  int16_t oF;  // forward, cm
  int16_t oR;  // right, cm
  int16_t oD;  // down, cm
} __uls_blitz_msg_eigc_nav_offset;

// Result of the GNSS compass, published by the module that computed it.
#define ULS_BLITZ_MSG_ID_EIGC_GNSS_COMPASS 0x0045
typedef struct __attribute__((packed)) {
  // Status :
  //  BIT   Description
  //  0     yaw valid
  //  1     baseline length within the configured tolerance
  //  2     epochs of the two fixes matched
  //  3     yaw ambiguity resolved against the magnetic heading
  //  4     solution is being published to the rest of the system
  //  5-7   reserved
  uint8_t status;
  uint8_t counter;
  // Platform yaw derived from the baseline between the two antennas.
  // deg = (value_int16 * 180.f)/32767.f
  int16_t yaw;
  uint16_t baseline;  // measured antenna separation, cm
  uint8_t yawAcc;     // accuracy estimate, 0.1 deg, saturating at 25.5 deg
  uint8_t peerLid;    // module this solution was formed with
} __uls_blitz_msg_eigc_gnss_compass;

// Vector from the sender antenna to the platform centre, in NED frame.
// Consumers add this to the sender position from the nav set to get the
// platform centre; that is the position handed to the drone over the BMD-345
// telemetry link so it can close on the platform.
#define ULS_BLITZ_MSG_ID_EIGC_PLATFORM_DELTA 0x0046
typedef struct __attribute__((packed)) {
  uint8_t status;
  uint8_t counter;  // matches the nav set this delta was computed against
  int16_t dN;       // cm
  int16_t dE;       // cm
  int16_t dD;       // cm
} __uls_blitz_msg_eigc_platform_delta;

//
// Common data structure for all messages.
//
typedef struct {
  uint8_t src_lid;
  uint16_t msg_id;
  union {
    uint8_t data[8];
    __uls_blitz_xcg1_rx_raw_measurmet xcg1_rx_raw_measurmet;
    __uls_blitz_msg_rx_raw_measurmet rx_raw_measurmet;
    __uls_blitz_msg_rx_pos_in_tx_frame rx_pos_in_tx_frame;
    __uls_blitz_msg_rx_mrx rx_mrx;
    __uls_blitz_msg_rx_dacs rx_dacs;
    __uls_blitz_msg_gimu_data gimu_data;
    __uls_blitz_msg_xx_txc_imu_override xx_txc_imu_override;
    __uls_blitz_msg_xx_txc_mag_override xx_txc_mag_override;
    __uls_blitz_msg_eigc_nav_epoch eigc_nav_epoch;
    __uls_blitz_msg_eigc_nav_lat eigc_nav_lat;
    __uls_blitz_msg_eigc_nav_lon eigc_nav_lon;
    __uls_blitz_msg_eigc_nav_vel eigc_nav_vel;
    __uls_blitz_msg_eigc_nav_offset eigc_nav_offset;
    __uls_blitz_msg_eigc_gnss_compass eigc_gnss_compass;
    __uls_blitz_msg_eigc_platform_delta eigc_platform_delta;
  };

} __blitz_common_data_struct;

#endif  // ULSBLITZTYPES_H
