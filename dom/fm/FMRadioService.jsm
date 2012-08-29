/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict"

let DEBUG = 0;
if (DEBUG)
  debug = function(s) { dump("-*- Fallback FMRadioService component: " + s + "\n");  };
else
  debug = function(s) {};

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

const kMozSettingsChangedObserverTopic = "mozsettings-changed";

const BAND_87500_108000_kHz = 1;
const BAND_76000_108000_kHz = 2;
const BAND_76000_90000_kHz  = 3;

const FM_BANDS = { };
FM_BANDS[BAND_76000_90000_kHz] = {
  lower: 76000,
  upper: 90000
};

FM_BANDS[BAND_87500_108000_kHz] = {
  lower: 87500,
  upper: 108000
};

FM_BANDS[BAND_76000_108000_kHz] = {
  lower: 76000,
  upper: 108000
};

const BAND_SETTING_KEY = "fm.band";
const SPACETYPE_SETTING_KEY = "fm.spaceType";

// Hal types
const RADIO_SEEK_DIRECTION_UP   = 0;
const RADIO_SEEK_DIRECTION_DOWN = 1;

const RADIO_CHANNEL_SPACE_TYPE_200KHZ = 200;
const RADIO_CHANNEL_SPACE_TYPE_100KHZ = 100;
const RADIO_CHANNEL_SPACE_TYPE_50KHZ  = 50;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/ctypes.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyGetter(this, "FMRadio", function() {
  return Cc["@mozilla.org/fmradio;1"].getService(Ci.nsIFMRadio);
});

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService", 
                            "@mozilla.org/settingsService;1",
                            "nsISettingsService");

XPCOMUtils.defineLazyGetter(this, "hiddenWindow", function() {
  let window = Cc["@mozilla.org/appshell/appShellService;1"]
                 .getService(Ci.nsIAppShellService)
                 .hiddenDOMWindow;
  return window;
});

let EXPORTED_SYMBOLS = ["FMRadioService"];

let _seeking = false;
let _seekingCallback = null;

function onSeekComplete() {
  if (_seeking) {
    FMRadio.removeEventListener("seekcomplete", onSeekComplete);
    _seeking = false;

    if (_seekingCallback) {
      _seekingCallback();
      _seekingCallback = null;
    }
  }
}


/**
 * Seek the next channel with given direction.
 * Only one seek action is allowed at once.
 */
function seekStation(direction, callback) {
  debug("seek station with direction " + direction);

  if (_seeking) {
    // Pass an negative number to callback which indicates that the seek action fails.
    callback(-1);
    return;
  }

  _seekingCallback = callback;
  _seeking = true;

  FMRadio.seek(direction);
  FMRadio.addEventListener("seekcomplete", onSeekComplete);
}

function cancelSeek() {
  if (_seeking) {
    FMRadio.cancelSeek();
    // No fail-to-seek or similar event will be fired from hal, so execute the seek
    // callback here manually.
    onSeekComplete();
    // The FM radio will stop at one frequency without any event, need to update current frequency
    FMRadioService._updateFrequency();
  }
}

/**
 * Round the frequency to match the range of frequency and the channel width.
 * If the given frequency is out of range, return null.
 * For example:
 *  - lower: 87.5MHz, upper: 108MHz, space: 0.2MHz
 *    87.6 is rounded to 87.7
 *    87.58 is runded to 87.5
 */
function roundFrequency(frequencyInKHz) {
  if (frequencyInKHz < FM_BANDS[_currentBand].lower ||
      frequencyInKHz > FM_BANDS[_currentBand].upper) {
    return null;
  }

  let roundedPart = frequencyInKHz - FM_BANDS[_currentBand].lower;
  roundedPart = Math.round(roundedPart / _currentSpaceType) * _currentSpaceType;
  frequencyInKHz = FM_BANDS[_currentBand].lower + roundedPart;
  return frequencyInKHz;
}

let _inited = false;
// Indicates if the FM radio is enabled currently
let _isEnabled = false;
// Indicates if the FM radio is being enabled currently
let _enabling = false;
// Current frequency in KHz
let _currentFrequency = 0;
// Current band setting
let _currentBand = BAND_87500_108000_kHz;
let _currentSpaceType = RADIO_CHANNEL_SPACE_TYPE_100KHZ;
let _currentSignalStrength = 0;
// Indicates if the antenna is available currently
let _antennaAvailable = true;

let FMRadioService = {
  init: function() {
    if (_inited === true) {
      return;
    }
    _inited = true;

    this._messages = ["DOMFMRadio:enable", "DOMFMRadio:disable",
                      "DOMFMRadio:setFrequency", "DOMFMRadio:getCurrentBand", "DOMFMRadio:getPowerState",
                      "DOMFMRadio:getFrequency", "DOMFMRadio:getSignalStrength", "DOMFMRadio:getAntennaState",
                      "DOMFMRadio:seekUp", "DOMFMRadio:seekDown", "DOMFMRadio:cancelSeek"
                     ];
    this._messages.forEach(function(msgName) {
      ppmm.addMessageListener(msgName, this);
    }.bind(this));

    Services.obs.addObserver(this, "profile-before-change", false);
    Services.obs.addObserver(this, kMozSettingsChangedObserverTopic, false);

    this._updatePowerState();

    // If the FM radio is enabled, update the current frequency immediately,
    // and start interval to check the signal strength.
    if (_isEnabled) {
      this._updateFrequency();
      this._startSignalChecking();
    }

    // Get the band setting
    gSettingsService.getLock().get(BAND_SETTING_KEY, this);

    // Get the space type setting
    gSettingsService.getLock().get(SPACETYPE_SETTING_KEY, this);

    this._updateAntennaState();

    let self = this;
    FMRadio.onantennastatechange = function onantennachange() {
      self._updateAntennaState();
    };

    debug("Inited");
  },

  // nsISettingsServiceCallback
  handle: function(aName, aResult) {
    if (aName == BAND_SETTING_KEY) {
      this._updateBand(aResult);
    } else if (aName == SPACETYPE_SETTING_KEY) {
      this._updateSpaceType(aResult);
    }
  },

  handleError: function(aErrorMessage) {
    this._updateBand(BAND_87500_108000_kHz);
    this._updateSpaceType(RADIO_CHANNEL_SPACE_TYPE_100KHZ);
  },

  _updateAntennaState: function() {
    let antennaState = FMRadio.isAntennaAvailable;
    let antennaChanged = antennaState != _antennaAvailable;
    _antennaAvailable = antennaState;

    if (antennaChanged) {
      ppmm.broadcastAsyncMessage("DOMFMRadio:antennaChange", { });
    }
  },

  _updateBand: function(band) {
      switch (parseInt(band)) {
        case BAND_87500_108000_kHz:
        case BAND_76000_108000_kHz:
        case BAND_76000_90000_kHz:
          _currentBand = band;
          break;
      }
  },

  _updateSpaceType: function(spaceType) {
    switch (parseInt(spaceType)) {
      case RADIO_CHANNEL_SPACE_TYPE_50KHZ:
      case RADIO_CHANNEL_SPACE_TYPE_100KHZ:
      case RADIO_CHANNEL_SPACE_TYPE_200KHZ:
        _currentSpaceType = spaceType;
        break;
    }
  },

  /**
   * Update the current signal strength.
   * Send strength change message if the signal strength is changed.
   */
  _updateSignalStrength: function() {
    let signalStrength = FMRadio.signalStrength;
    let strengthChanged = _currentSignalStrength != signalStrength;
    _currentSignalStrength = signalStrength;

    if (strengthChanged) {
      ppmm.broadcastAsyncMessage("DOMFMRadio:signalStrengthChange", { });
    }
  },

  /**
   * Update and cache the current frequency.
   * Send frequency change message if the frequency is changed.
   */
  _updateFrequency: function(callback) {
    let frequency = FMRadio.frequency;
    let frequencyChanged = frequency != _currentFrequency;
    _currentFrequency = frequency;

    if (frequencyChanged) {
      ppmm.broadcastAsyncMessage("DOMFMRadio:frequencyChange", { });
    }

    if (typeof callback == "function") {
      callback();
    }
  },

  /**
   * Update and cache the power state of the FM radio.
   * Send message if the power state is changed.
   */
  _updatePowerState: function() {
    let enabled = FMRadio.enabled;
    let powerStateChanged = _isEnabled != enabled;
    _isEnabled = enabled;

    if (powerStateChanged) {
      ppmm.broadcastAsyncMessage("DOMFMRadio:powerStateChange", { });
    }
  },

  /**
   * Start an interval to check the signal strength.
   */
  _startSignalChecking: function() {
    this._stopSignalChecking();

    var self = this;
    this._signalCheckingInterval = hiddenWindow.setInterval(function checkSignal() {
      self._updateSignalStrength();
    }, 2000);
  },

  _stopSignalChecking: function() {
    if (this._signalCheckingInterval != null) {
      hiddenWindow.clearInterval(this._signalCheckingInterval);
      this._signalCheckingInterval = null;
    }
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "profile-before-change":
        this._messages.forEach(function(msgName) {
          ppmm.removeMessageListener(msgName, this);
        }.bind(this));

        Services.obs.removeObserver(this, "profile-before-change");
        Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);

        ppmm = null;
        this._messages = null;
        break;
      case kMozSettingsChangedObserverTopic:
        let setting = JSON.parse(aData);
        this.handleMozSettingsChanged(setting);
        break;
    }
  },

  _sendMessage: function(message, success, data, msg) {
    msg.manager.sendAsyncMessage(message + (success ? ":OK" : ":NO"), {
      data: data,
      rid: msg.rid,
      mid: msg.mid
    });
  },

  handleMozSettingsChanged: function(settings) {
    switch (settings.key) {
      case BAND_SETTING_KEY:
        this._updateBand(settings.value);
        break;
      case SPACETYPE_SETTING_KEY:
        this._updateSpaceType(settings.value);
        break;
    }
  },

  receiveMessage: function(aMessage) {
    let msg = aMessage.json || {};
    msg.manager = aMessage.target;

    let ret = 0;
    let self = this;
    switch (aMessage.name) {
      case "DOMFMRadio:enable": {
        let frequencyInKHz = roundFrequency(parseInt(msg.data * 1000));
        // If the FM radio is already enabled or it is being enabled currently or the
        // given frequency is out of range, return false.
        if (_isEnabled || _enabling || !frequencyInKHz) {
          self._sendMessage("DOMFMRadio:enable:Return", false, null, msg);
        } else {
          _enabling = true;

          FMRadio.enable({
            lowerLimit: FM_BANDS[_currentBand].lower,
            upperLimit: FM_BANDS[_currentBand].upper,
            spaceType:  _currentSpaceType   // 100KHz by default
          });

          FMRadio.addEventListener("enabled", function on_enabled() {
            debug("FM Radio is enabled!");
            FMRadio.removeEventListener("enabled", on_enabled);
            _enabling = false;

            self._updatePowerState();
            self._sendMessage("DOMFMRadio:enable:Return", true, true, msg);

            // Set frequency with the given frequency after the enable action completes.
            FMRadio.setFrequency(frequencyInKHz);
            self._updateFrequency();

            // Start to check signal strength after the enable action completes.
            self._startSignalChecking();
          });
        }
        break;
      }
      case "DOMFMRadio:disable":
        // If the FM radio is already disabled, return false.
        if (!_isEnabled) {
          self._sendMessage("DOMFMRadio:disable:Return", false, null, msg);
        } else {
          FMRadio.disable();
          FMRadio.addEventListener("disabled", function on_disabled() {
            debug("FM Radio is disabled!");
            FMRadio.removeEventListener("disabled", on_disabled);

            self._updatePowerState();
            self._sendMessage("DOMFMRadio:disable:Return", true, false, msg);

            self._stopSignalChecking();

            // If the FM Radio is seeking currently, no fail-to-seek or similar
            // event will be fired, execute the seek callback manually.
            onSeekComplete();
          });
        }
        break;
      case "DOMFMRadio:setFrequency": {
        let frequencyInKHz = roundFrequency(parseInt(msg.data * 1000));
        // If the FM radio is disabled or the given frequency is out of ranges, return false.
        if (!_isEnabled || !frequencyInKHz) {
          self._sendMessage("DOMFMRadio:setFrequency:Return", false, null, msg);
        } else {
          FMRadio.setFrequency(frequencyInKHz);
          self._sendMessage("DOMFMRadio:setFrequency:Return", true, null, msg);
          this._updateFrequency();
        }
        break;
      }
      case "DOMFMRadio:getCurrentBand":
        // this message is sync
        return {
          lower: FM_BANDS[_currentBand].lower / 1000,
          upper: FM_BANDS[_currentBand].upper / 1000,
          channelWidth: _currentSpaceType / 1000
        };
      case "DOMFMRadio:getPowerState":
        // this message is sync
        return _isEnabled;
      case "DOMFMRadio:getFrequency":
        // this message is sync
        return _isEnabled ? _currentFrequency / 1000 : null;    // unit MHz
      case "DOMFMRadio:getSignalStrength":
        // this message is sync
        return _currentSignalStrength;
      case "DOMFMRadio:getAntennaState":
        // this message is sync
        return _antennaAvailable;
      case "DOMFMRadio:seekDown":
        // If the FM radio is disabled, do not execute the seek action.
        if (!_isEnabled) {
          self._sendMessage("DOMFMRadio:seekDown:Return", false, null, msg);
        } else {
          seekStation(RADIO_SEEK_DIRECTION_DOWN, function(frequency) {
            debug("Seek down completes.");
            if (!!frequency && frequency < 0) {
              self._sendMessage("DOMFMRadio:seekDown:Return", false, null, msg);
            } else {
              // Make sure the FM app will get the right frequency.
              self._updateFrequency(function() {
                self._sendMessage("DOMFMRadio:seekDown:Return", true, null, msg);
              });
            }
          });
        }
        break;
      case "DOMFMRadio:seekUp":
        // If the FM radio is disabled, do not execute the seek action.
        if(!_isEnabled) {
          self._sendMessage("DOMFMRadio:seekUp:Return", false, null, msg);
        } else {
          seekStation(RADIO_SEEK_DIRECTION_UP, function(frequency) {
            debug("Seek up completes.");
            if (!!frequency && frequency < 0) {
              self._sendMessage("DOMFMRadio:seekUp:Return", false, null, msg);
            } else {
              // Make sure the FM app will get the right frequency.
              self._updateFrequency(function() {
                self._sendMessage("DOMFMRadio:seekUp:Return", true, null, msg);
              });
            }
          });
        }
        break;
      case "DOMFMRadio:cancelSeek":
        // If the FM radio is disabled, do not execute the cancel seek action.
        if (!_isEnabled) {
          self._sendMessage("DOMFMRadio:cancelSeek:Return", false, null, msg);
        } else {
          cancelSeek();
          // Make sure the FM app will get the right frequency.
          self._updateFrequency(function() {
             self._sendMessage("DOMFMRadio:cancelSeek:Return", true, true, msg);
          })
        }
        break;
    }
  }
};

FMRadioService.init();

