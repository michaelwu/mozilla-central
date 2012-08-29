/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=40: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_fm_radio_h__
#define mozilla_dom_fm_radio_h__

#include "nsCOMPtr.h"
#include "mozilla/HalTypes.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIFMRadio.h"

#define NS_FMRADIO_CONTRACTID "@mozilla.org/fmradio;1"
// 9cb91834-78a9-4029-b644-7806173c5e2d
#define NS_FMRADIO_CID {0x9cb91834, 0x78a9, 0x4029, \
      {0xb6, 0x44, 0x78, 0x06, 0x17, 0x3c, 0x5e, 0x2d}}

using namespace mozilla::hal;

namespace mozilla {
namespace hal {
class SwitchEvent;
typedef Observer<SwitchEvent> SwitchObserver;
} // namespace hal

namespace dom {
namespace fm {

/* Header file */
class FMRadio : public nsDOMEventTargetHelper,
                  public nsIFMRadio,
                  public FMRadioObserver,
                  public mozilla::hal::SwitchObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFMRADIO

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(
                                                   FMRadio,
                                                   nsDOMEventTargetHelper)

  FMRadio();

private:
  ~FMRadio();
  bool mHasInternalAntenna;
  /**
   * Dispatch a trusted non-cancellable and no-bubbling event to itself
   */
  nsresult DispatchTrustedEventToSelf(const nsAString& aEventName);

protected:
  NS_DECL_EVENT_HANDLER(seekcomplete)
  NS_DECL_EVENT_HANDLER(enabled)
  NS_DECL_EVENT_HANDLER(disabled)
  NS_DECL_EVENT_HANDLER(antennastatechange)
  int mStatus;
  nsAutoPtr<mozilla::hal::SwitchObserver> mObserver;

public:
  void Notify(const FMRadioOperationInformation& info);
  void Notify(const mozilla::hal::SwitchEvent& aEvent);
};

} // namespace fm
} // namespace dom
} // namespace mozilla
#endif

