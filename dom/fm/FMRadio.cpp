/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <android/log.h>

#include "mozilla/Hal.h"
#include "mozilla/HalTypes.h"
#include "mozilla/Preferences.h"
#include "AudioManager.h"
#include "nsIAudioManager.h"
#include "FMRadio.h"
#include "nsDOMEvent.h"
#include "nsDOMClassInfo.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "FMRadio" , ## args)

// The pref which indicates if the device has an internal antenna. 
// If the pref is true, the antanna will be always available.
#define DOM_FM_ANTENNA_INTERNAL_PREF "dom.fm.antenna.internal"

#define RADIO_SEEK_COMPLETE_EVENT_NAME NS_LITERAL_STRING("seekcomplete")
#define RADIO_DIABLED_EVENT_NAME NS_LITERAL_STRING("disabled")
#define RADIO_ENABLED_EVENT_NAME NS_LITERAL_STRING("enabled")
#define ANTENNA_STATE_CHANGED_EVENT_NAME NS_LITERAL_STRING("antennastatechange")

using namespace mozilla::dom::fm;
using namespace mozilla::hal;
using namespace mozilla::dom::gonk;
using mozilla::Preferences;

int32_t GetJsIntValue(JSContext* cx, JSObject* obj, const char *name)
{
  jsval jsVal;
  JS_GetProperty(cx, obj, name, &jsVal);

  int32_t value;
  NS_ENSURE_TRUE(JS_ValueToInt32(cx, jsVal, &value), NS_ERROR_FAILURE);

  return value;
}

/* Implementation file */
FMRadio::FMRadio(): mStatus(1), mHasInternalAntenna(false)
{
  LOG("FMRadio is initialized.");

  if (!NS_SUCCEEDED(Preferences::GetBool(DOM_FM_ANTENNA_INTERNAL_PREF,
            &mHasInternalAntenna)) || !mHasInternalAntenna)
  {
    RegisterSwitchObserver(SWITCH_HEADPHONES, this);
    mStatus = GetCurrentSwitchState(SWITCH_HEADPHONES);
  }
  else
  {
    LOG("We have an internal antenna.");
  }

  RegisterFMRadioObserver(this);
}

FMRadio::~FMRadio()
{
  UnregisterFMRadioObserver(this);
  if (!mHasInternalAntenna)
  {
    UnregisterSwitchObserver(SWITCH_HEADPHONES, this);
  }
}

DOMCI_DATA(FMRadio, FMRadio)

NS_IMPL_CYCLE_COLLECTION_CLASS(FMRadio)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(FMRadio,
                                                  nsDOMEventTargetHelper)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(seekcomplete)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(disabled)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(enabled)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(antennastatechange)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(FMRadio,
                                               nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(FMRadio,
                                                nsDOMEventTargetHelper)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(seekcomplete)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(disabled)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(enabled)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(antennastatechange)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(FMRadio)
  NS_INTERFACE_MAP_ENTRY(nsIFMRadio)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(FMRadio)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_EVENT_HANDLER(FMRadio, seekcomplete)
NS_IMPL_EVENT_HANDLER(FMRadio, disabled)
NS_IMPL_EVENT_HANDLER(FMRadio, enabled)
NS_IMPL_EVENT_HANDLER(FMRadio, antennastatechange)

NS_IMPL_ADDREF_INHERITED(FMRadio, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(FMRadio, nsDOMEventTargetHelper)

/* readonly attribute boolean isAntennaAvailable; */
NS_IMETHODIMP FMRadio::GetIsAntennaAvailable(bool *aIsAvailable)
{
  if (mHasInternalAntenna)
    *aIsAvailable = true;
  else
    *aIsAvailable = mStatus == 0;
  return NS_OK;
}

/* readonly attribute long frequency; */
NS_IMETHODIMP FMRadio::GetFrequency(PRInt32 *aFrequency)
{
  GetFMRadioFrequency(aFrequency);
  return NS_OK;
}

/* readonly attribute long signalStrength; */
NS_IMETHODIMP FMRadio::GetSignalStrength(float *aSignalStrength)
{
  uint32_t signalStrength;

  // The range of signal strengh is [0-100], higher than 100 means signal is strong.
  GetFMRadioSignalStrength(&signalStrength);

  *aSignalStrength = (signalStrength > 100 ? 100 : signalStrength) / 100.0;

  return NS_OK;
}

/* readonly attribute blean enabled; */
NS_IMETHODIMP FMRadio::GetEnabled(bool *aEnabled)
{
  *aEnabled = IsFMRadioOn();
  return NS_OK;
}

/* [implicit_jscontext] void enableRadio (in jsval settings); */
NS_IMETHODIMP FMRadio::Enable(const JS::Value & settings, JSContext* cx)
{
  if (!settings.isObject()) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  JSObject *obj = &settings.toObject();

  int32_t upperLimit = GetJsIntValue(cx, obj, "upperLimit");
  int32_t lowerLimit = GetJsIntValue(cx, obj, "lowerLimit");
  int32_t spaceType  = GetJsIntValue(cx, obj, "spaceType");

  hal::FMRadioSettings info;
  info.upperLimit() = upperLimit;
  info.lowerLimit() = lowerLimit;
  info.spaceType() = spaceType;

  AudioManager::EnableFM();

  EnableFMRadio(info);

  return NS_OK;
}

/* void disableRadio (); */
NS_IMETHODIMP FMRadio::Disable()
{
  AudioManager::DisableFM();
  DisableFMRadio();
  return NS_OK;
}

/* void cancelSeek */
NS_IMETHODIMP FMRadio::CancelSeek()
{
  CancelFMRadioSeek();
  return NS_OK;
}

/* void seek (in short direction); */
NS_IMETHODIMP FMRadio::Seek(PRInt16 direction)
{
  if (direction == (int)FM_RADIO_SEEK_DIRECTION_UP) {
    FMRadioSeek(FM_RADIO_SEEK_DIRECTION_UP);
  } else {
    FMRadioSeek(FM_RADIO_SEEK_DIRECTION_DOWN);
  }
  return NS_OK;
}

/* [implicit_jscontext] jsval getSettings (); */
NS_IMETHODIMP FMRadio::GetSettings(JSContext* cx, JS::Value *_retval)
{
  FMRadioSettings settings;
  GetFMRadioSettings(&settings);

  JSObject* jsVal = JS_NewObject(cx, nullptr, nullptr, nullptr);

  jsval upperLimit = JS_NumberValue(settings.upperLimit());
  JS_SetProperty(cx, jsVal, "upperLimit", &upperLimit);

  jsval lowerLimit = JS_NumberValue(settings.lowerLimit());
  JS_SetProperty(cx, jsVal, "lowerLimit", &lowerLimit);

  jsval spaceType = JS_NumberValue((uint32_t)settings.spaceType());
  JS_SetProperty(cx, jsVal, "spaceType", &spaceType);

  *_retval = OBJECT_TO_JSVAL(jsVal);

  return NS_OK;
}

/* void setFrequency (in short frequency); */
NS_IMETHODIMP FMRadio::SetFrequency(PRInt32 frequency)
{
  SetFMRadioFrequency(frequency);
  return NS_OK;
}

void FMRadio::Notify(const SwitchEvent& aEvent)
{
  int preStatus = mStatus;
  mStatus = aEvent.status();

  if (preStatus != mStatus) {
    LOG("Antenna state is changed!");
    DispatchTrustedEventToSelf(ANTENNA_STATE_CHANGED_EVENT_NAME);
  }
}

void FMRadio::Notify(const FMRadioOperationInformation& info)
{
  switch (info.operation())
  {
    case FM_RADIO_OPERATION_ENABLE:
      DispatchTrustedEventToSelf(RADIO_ENABLED_EVENT_NAME);
      break;
    case FM_RADIO_OPERATION_DISABLE:
      DispatchTrustedEventToSelf(RADIO_DIABLED_EVENT_NAME);
      break;
    case FM_RADIO_OPERATION_SEEK:
      DispatchTrustedEventToSelf(RADIO_SEEK_COMPLETE_EVENT_NAME);
      break;
  }
}

nsresult
FMRadio::DispatchTrustedEventToSelf(const nsAString& aEventName)
{
  nsRefPtr<nsDOMEvent> event = new nsDOMEvent(nullptr, nullptr);
  nsresult rv = event->InitEvent(aEventName, false, false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = event->SetTrusted(true);
  NS_ENSURE_SUCCESS(rv, rv);

  bool dummy;
  rv = DispatchEvent(event, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

