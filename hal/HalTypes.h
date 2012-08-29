/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_hal_Types_h
#define mozilla_hal_Types_h

#include "IPCMessageUtils.h"

namespace mozilla {
namespace hal {

/**
 * These are defined by libhardware, specifically, hardware/libhardware/include/hardware/lights.h
 * in the gonk subsystem.
 * If these change and are exposed to JS, make sure nsIHal.idl is updated as well.
 */
enum LightType {
    eHalLightID_Backlight = 0,
    eHalLightID_Keyboard = 1,
    eHalLightID_Buttons = 2,
    eHalLightID_Battery = 3,
    eHalLightID_Notifications = 4,
    eHalLightID_Attention = 5,
    eHalLightID_Bluetooth = 6,
    eHalLightID_Wifi = 7,
    eHalLightID_Count = 8         // This should stay at the end
};
enum LightMode {
    eHalLightMode_User = 0,       // brightness is managed by user setting
    eHalLightMode_Sensor = 1,     // brightness is managed by a light sensor
    eHalLightMode_Count
};
enum FlashMode {
    eHalLightFlash_None = 0,
    eHalLightFlash_Timed = 1,     // timed flashing.  Use flashOnMS and flashOffMS for timing
    eHalLightFlash_Hardware = 2,  // hardware assisted flashing
    eHalLightFlash_Count
};

class SwitchEvent;

enum SwitchDevice {
  SWITCH_DEVICE_UNKNOWN = -1,
  SWITCH_HEADPHONES,
  SWITCH_USB,
  NUM_SWITCH_DEVICE
};

enum SwitchState {
  SWITCH_STATE_UNKNOWN = -1,
  SWITCH_STATE_ON,
  SWITCH_STATE_OFF,
  NUM_SWITCH_STATE
};

typedef Observer<SwitchEvent> SwitchObserver;

enum ProcessPriority {
  PROCESS_PRIORITY_BACKGROUND,
  PROCESS_PRIORITY_FOREGROUND,
  PROCESS_PRIORITY_MASTER,
  NUM_PROCESS_PRIORITY
};

/**
 * Used by ModifyWakeLock
 */
enum WakeLockControl {
  WAKE_LOCK_REMOVE_ONE = -1,
  WAKE_LOCK_NO_CHANGE  = 0,
  WAKE_LOCK_ADD_ONE    = 1,
  NUM_WAKE_LOCK
};

class FMRadioOperationInformation;

enum FMRadioOperation {
  FM_RADIO_OPERATION_UNKNOWN = -1,
  FM_RADIO_OPERATION_ENABLE,
  FM_RADIO_OPERATION_DISABLE,
  FM_RADIO_OPERATION_SEEK,
  NUM_FM_RADIO_OPERATION
};

enum FMRadioOperationStatus {
  FM_RADIO_OPERATION_STATUS_UNKNOWN = -1,
  FM_RADIO_OPERATION_STATUS_SUCCESS,
  FM_RADIO_OPERATION_STATUS_FAIL,
  NUM_FM_RADIO_OPERATION_STATUS
};

enum FMRadioSeekDirection {
  FM_RADIO_SEEK_DIRECTION_UNKNOWN = -1,
  FM_RADIO_SEEK_DIRECTION_UP,
  FM_RADIO_SEEK_DIRECTION_DOWN,
  NUM_FM_RADIO_SEEK_DIRECTION
};

enum FMRadioBandType {
  FM_RADIO_BAND_TYPE_UNKNOWN = -1,
  FM_RADIO_BAND_TYPE_US,  //USA
  FM_RADIO_BAND_TYPE_EU,
  FM_RADIO_BAND_TYPE_JP_STANDARD,
  FM_RADIO_BAND_TYPE_JP_WIDE,
  FM_RADIO_BAND_TYPE_DE,  //Germany
  FM_RADIO_BAND_TYPE_AW,  //Aruba
  FM_RADIO_BAND_TYPE_AU,  //Australlia
  FM_RADIO_BAND_TYPE_BS,  //Bahamas
  FM_RADIO_BAND_TYPE_BD,  //Bangladesh
  FM_RADIO_BAND_TYPE_CY,  //Cyprus
  FM_RADIO_BAND_TYPE_VA,  //Vatican
  FM_RADIO_BAND_TYPE_CO,  //Colombia
  FM_RADIO_BAND_TYPE_KR,  //Korea
  FM_RADIO_BAND_TYPE_DK,  //Denmark
  FM_RADIO_BAND_TYPE_EC,  //Ecuador
  FM_RADIO_BAND_TYPE_ES,  //Spain
  FM_RADIO_BAND_TYPE_FI,  //Finland
  FM_RADIO_BAND_TYPE_FR,  //France
  FM_RADIO_BAND_TYPE_GM,  //Gambia
  FM_RADIO_BAND_TYPE_HU,  //Hungary
  FM_RADIO_BAND_TYPE_IN,  //India
  FM_RADIO_BAND_TYPE_IR,  //Iran
  FM_RADIO_BAND_TYPE_IT,  //Italy
  FM_RADIO_BAND_TYPE_KW,  //Kuwait
  FM_RADIO_BAND_TYPE_LT,  //Lithuania
  FM_RADIO_BAND_TYPE_ML,  //Mali
  FM_RADIO_BAND_TYPE_MA,  //Morocco
  FM_RADIO_BAND_TYPE_NO,  //Norway
  FM_RADIO_BAND_TYPE_NZ,  //New Zealand
  FM_RADIO_BAND_TYPE_OM,  //Oman
  FM_RADIO_BAND_TYPE_PG,  //Papua New Guinea
  FM_RADIO_BAND_TYPE_NL,  //Netherlands
  FM_RADIO_BAND_TYPE_QA,  //Qatar
  FM_RADIO_BAND_TYPE_CZ,  //Czech Republic
  FM_RADIO_BAND_TYPE_UK,  //United Kingdom of Great Britain and Northern Ireland
  FM_RADIO_BAND_TYPE_RW,  //Rwandese Republic
  FM_RADIO_BAND_TYPE_SN,  //Senegal
  FM_RADIO_BAND_TYPE_SG,  //Singapore
  FM_RADIO_BAND_TYPE_SI,  //Slovenia
  FM_RADIO_BAND_TYPE_ZA,  //South Africa
  FM_RADIO_BAND_TYPE_SE,  //Sweden
  FM_RADIO_BAND_TYPE_CH,  //Switzerland
  FM_RADIO_BAND_TYPE_TW,  //Taiwan
  FM_RADIO_BAND_TYPE_TR,  //Turkey
  FM_RADIO_BAND_TYPE_UA,  //Ukraine
  FM_RADIO_BAND_TYPE_USER_DEFINED,
  NUM_FM_RADIO_BAND_TYPE
};

typedef Observer<FMRadioOperationInformation> FMRadioObserver;

} // namespace hal
} // namespace mozilla

namespace IPC {

/**
 * Light type serializer.
 */
template <>
struct ParamTraits<mozilla::hal::LightType>
  : public EnumSerializer<mozilla::hal::LightType,
                          mozilla::hal::eHalLightID_Backlight,
                          mozilla::hal::eHalLightID_Count>
{};

/**
 * Light mode serializer.
 */
template <>
struct ParamTraits<mozilla::hal::LightMode>
  : public EnumSerializer<mozilla::hal::LightMode,
                          mozilla::hal::eHalLightMode_User,
                          mozilla::hal::eHalLightMode_Count>
{};

/**
 * Flash mode serializer.
 */
template <>
struct ParamTraits<mozilla::hal::FlashMode>
  : public EnumSerializer<mozilla::hal::FlashMode,
                          mozilla::hal::eHalLightFlash_None,
                          mozilla::hal::eHalLightFlash_Count>
{};

/**
 * WakeLockControl serializer.
 */
template <>
struct ParamTraits<mozilla::hal::WakeLockControl>
  : public EnumSerializer<mozilla::hal::WakeLockControl,
                          mozilla::hal::WAKE_LOCK_REMOVE_ONE,
                          mozilla::hal::NUM_WAKE_LOCK>
{};

/**
 * Serializer for SwitchState
 */
template <>
struct ParamTraits<mozilla::hal::SwitchState>:
  public EnumSerializer<mozilla::hal::SwitchState,
                        mozilla::hal::SWITCH_STATE_UNKNOWN,
                        mozilla::hal::NUM_SWITCH_STATE> {
};

/**
 * Serializer for SwitchDevice
 */
template <>
struct ParamTraits<mozilla::hal::SwitchDevice>:
  public EnumSerializer<mozilla::hal::SwitchDevice,
                        mozilla::hal::SWITCH_DEVICE_UNKNOWN,
                        mozilla::hal::NUM_SWITCH_DEVICE> {
};

template <>
struct ParamTraits<mozilla::hal::ProcessPriority>:
  public EnumSerializer<mozilla::hal::ProcessPriority,
                        mozilla::hal::PROCESS_PRIORITY_BACKGROUND,
                        mozilla::hal::NUM_PROCESS_PRIORITY> {
};

/**
 * Serializer for FMRadioOperation
 */
template <>
struct ParamTraits<mozilla::hal::FMRadioOperation>:
  public EnumSerializer<mozilla::hal::FMRadioOperation,
                        mozilla::hal::FM_RADIO_OPERATION_UNKNOWN,
                        mozilla::hal::NUM_FM_RADIO_OPERATION> {
};

/**
 * Serializer for FMRadioOperationStatus
 */
template <>
struct ParamTraits<mozilla::hal::FMRadioOperationStatus>:
  public EnumSerializer<mozilla::hal::FMRadioOperationStatus,
                        mozilla::hal::FM_RADIO_OPERATION_STATUS_UNKNOWN,
                        mozilla::hal::NUM_FM_RADIO_OPERATION_STATUS> {
};

/**
 * Serializer for FMRadioSeekDirection
 */
template <>
struct ParamTraits<mozilla::hal::FMRadioSeekDirection>:
  public EnumSerializer<mozilla::hal::FMRadioSeekDirection,
                        mozilla::hal::FM_RADIO_SEEK_DIRECTION_UNKNOWN,
                        mozilla::hal::NUM_FM_RADIO_SEEK_DIRECTION> {
};

/**
 * Serializer for FMRadioBandType
 **/
template <>
struct ParamTraits<mozilla::hal::FMRadioBandType>:
  public EnumSerializer<mozilla::hal::FMRadioBandType,
                        mozilla::hal::FM_RADIO_BAND_TYPE_UNKNOWN,
                        mozilla::hal::NUM_FM_RADIO_BAND_TYPE> {
};
} // namespace IPC

#endif // mozilla_hal_Types_h
