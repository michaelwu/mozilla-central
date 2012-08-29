/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set shiftwidth=2 tabstop=2 autoindent cindent expandtab: */

/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. 
 */

"use strict"
let DEBUG = 0;
if (DEBUG)
  debug = function (s) { dump("-*- DOMFMRadio: " + s + "\n"); };
else
  debug = function (s) { };

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");

const DOMFMMANAGER_CONTRACTID = "@mozilla.org/domfmradio;1";
const DOMFMMANAGER_CID        = Components.ID("{901f8a83-03a6-4be9-bb8f-35387d3849da}");

XPCOMUtils.defineLazyGetter(Services, "DOMRequest", function() {
  return Cc["@mozilla.org/dom/dom-request-service;1"].getService(Ci.nsIDOMRequestService);
});

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

function DOMFMRadio() { }

DOMFMRadio.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  classID: DOMFMMANAGER_CID,
  classInfo: XPCOMUtils.generateCI({
               classID: DOMFMMANAGER_CID,
               contractID: DOMFMMANAGER_CONTRACTID,
               classDescription: "DOMFMRadio",
               interfaces: [Ci.nsIDOMFMRadio],
               flags: Ci.nsIClassInfo.DOM_OBJECT
             }),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMFMRadio, Ci.nsIDOMGlobalPropertyInitializer]),

  // nsIDOMGlobalPropertyInitializer implementation
  init: function(aWindow) {
    let principal = aWindow.document.nodePrincipal;
    let secMan = Cc["@mozilla.org/scriptsecuritymanager;1"].getService(Ci.nsIScriptSecurityManager);

    let perm = (principal == secMan.getSystemPrincipal()) ?
                 Ci.nsIPermissionManager.ALLOW_ACTION :
                 Services.perms.testExactPermission(principal.URI, "fmradio");
    this._hasPrivileges = perm == Ci.nsIPermissionManager.ALLOW_ACTION;

    const messages = ["DOMFMRadio:enable:Return:OK", "DOMFMRadio:enable:Return:NO",
                      "DOMFMRadio:disable:Return:OK", "DOMFMRadio:disable:Return:NO",
                      "DOMFMRadio:setFrequency:Return:OK", "DOMFMRadio:setFrequency:Return:NO",
                      "DOMFMRadio:seekUp:Return:OK", "DOMFMRadio:seekUp:Return:NO",
                      "DOMFMRadio:seekDown:Return:OK", "DOMFMRadio:seekDown:Return:NO",
                      "DOMFMRadio:cancelSeek:Return:OK", "DOMFMRadio:cancelSeek:Return:NO",
                      "DOMFMRadio:frequencyChange", "DOMFMRadio:powerStateChange",
                      "DOMFMRadio:signalStrengthChange", "DOMFMRadio:getAntennaState", "DOMFMRadio:antennaChange"];
    this.initHelper(aWindow, messages);
  },

  // Called from DOMRequestIpcHelper
  uninit: function() {
    this._onFrequencyChange = null;
    this._onAntennaChange = null;
    this._onDisabled = null;
    this._onEnabled = null;
    this._onSignalChange = null;
  },

  _createEvent: function(name) {
    return new this._window.Event(name);
  },

  _sendMessageForRequest: function(name, data, request) {
    let id = this.getRequestId(request);
    cpmm.sendAsyncMessage(name, {
      data: data,
      rid: id,
      mid: this._id
    });
  },

  _fireFrequencyChangeEvent: function() {
    let e = this._createEvent("frequencychange");
    if (this._onFrequencyChange) {
      this._onFrequencyChange.handleEvent(e);
    }
    this.dispatchEvent(e);
  },

  _firePowerStateChangeEvent: function() {
    let _enabled = this.enabled;
    debug("Current power state: " + _enabled);
    if (_enabled) {
      let e = this._createEvent("enabled");
      if (this._onEnabled) {
        this._onEnabled.handleEvent(e);
      }
      this.dispatchEvent(e);
    } else {
      let e = this._createEvent("disabled");
      if (this._onDisabled) {
        this._onDisabled.handleEvent(e);
      }
      this.dispatchEvent(e);
    }
  },

  _fireSignalChangeEvent: function() {
    let e = this._createEvent("signalstrengthchange");
    if (this._onSignalChange) {
      this._onSignalChange.handleEvent(e);
    }
    this.dispatchEvent(e);
  },

  _fireAntennaChangeEvent: function() {
    let e = this._createEvent("antennaavailablechange");
    if (this._onAntennaChange) {
      this._onAntennaChange.handleEvent(e);
    }
    this.dispatchEvent(e);
  },

  receiveMessage: function(aMessage) {
    let msg = aMessage.json;
    if (msg.mid && msg.mid != this._id) {
      return;
    }

    let request;
    switch (aMessage.name) {
      case "DOMFMRadio:enable:Return:OK":
        request = this.takeRequest(msg.rid);
        Services.DOMRequest.fireSuccess(request, msg.data);
        break;
      case "DOMFMRadio:enable:Return:NO":
        request = this.takeRequest(msg.rid);
        Services.DOMRequest.fireError(request, "Failed to turn on FM");
        break;
      case "DOMFMRadio:disable:Return:OK":
        request = this.takeRequest(msg.rid);
        Services.DOMRequest.fireSuccess(request, msg.data);
        break;
      case "DOMFMRadio:disable:Return:NO":
        request = this.takeRequest(msg.rid);
        Services.DOMRequest.fireError(request, "Failed to turn off FM");
        break;
      case "DOMFMRadio:setFrequency:Return:OK":
        request = this.takeRequest(msg.rid);
        Services.DOMRequest.fireSuccess(request, true);
        break;
      case "DOMFMRadio:setFrequency:Return:NO":
        request = this.takeRequest(msg.rid);
        Services.DOMRequest.fireError(request, "Failed to set FM frequency");
        break;
      case "DOMFMRadio:seekUp:Return:OK":
        request = this.takeRequest(msg.rid);
        Services.DOMRequest.fireSuccess(request, true);
        break;
      case "DOMFMRadio:seekUp:Return:NO":
        request = this.takeRequest(msg.rid);
        Services.DOMRequest.fireError(request, "Failed to seek up frequency");
        break;
      case "DOMFMRadio:seekDown:Return:OK":
        request = this.takeRequest(msg.rid);
        Services.DOMRequest.fireSuccess(request, true);
        break;
      case "DOMFMRadio:seekDown:Return:NO":
        request = this.takeRequest(msg.rid);
        Services.DOMRequest.fireError(request, "Failed to seek down frequency");
        break;
      case "DOMFMRadio:cancelSeek:Return:OK":
        request = this.takeRequest(msg.rid);
        Services.DOMRequest.fireSuccess(request, true);
        break;
      case "DOMFMRadio:cancelSeek:Return:NO":
        request = this.takeRequest(msg.rid);
        Services.DOMRequest.fireError(request, "Failed to cancel seek");
        break;
      case "DOMFMRadio:powerStateChange":
        this._firePowerStateChangeEvent();
        break;
      case "DOMFMRadio:frequencyChange":
        this._fireFrequencyChangeEvent();
        break;
      case "DOMFMRadio:signalStrengthChange":
        this._fireSignalChangeEvent();
        break;
      case "DOMFMRadio:antennaChange":
        this._fireAntennaChangeEvent();
        break;
    }
  },

  _checkPermission: function() {
    if (!this._hasPrivileges) {
      throw new Components.Exception("Denied", Cr.NS_ERROR_FAILURE);
    }
  },

  _call: function(name, arg) {
    this._checkPermission();
    var request = this.createRequest();
    this._sendMessageForRequest("DOMFMRadio:" + name, arg, request);
    return request;
  },

  // nsIDOMFMRadio
  get enabled() {
    this._checkPermission();
    return cpmm.sendSyncMessage("DOMFMRadio:getPowerState")[0];
  },

  get antennaAvailable() {
    this._checkPermission();
    return cpmm.sendSyncMessage("DOMFMRadio:getAntennaState")[0];
  },

  get frequency() {
    this._checkPermission();
    return cpmm.sendSyncMessage("DOMFMRadio:getFrequency")[0];
  },

  get frequencyUpperBound() {
    let range = cpmm.sendSyncMessage("DOMFMRadio:getCurrentBand")[0];
    return range.upper;
  },

  get frequencyLowerBound() {
    let range = cpmm.sendSyncMessage("DOMFMRadio:getCurrentBand")[0];
    return range.lower;
  },

  get channelWidth() {
    let range = cpmm.sendSyncMessage("DOMFMRadio:getCurrentBand")[0];
    return range.channelWidth;
  },

  get signalStrength() {
    this._checkPermission();
    return cpmm.sendSyncMessage("DOMFMRadio:getSignalStrength")[0];
  },

  set onsignalstrengthchange(callback) {
    this._checkPermission();
    this._onSignalChange = callback;
  },

  set onantennaavailablechange(callback) {
    this._checkPermission();
    this._onAntennaChange = callback;
  },

  set onenabled(callback) {
    this._checkPermission();
    this._onEnabled = callback;
  },

  set ondisabled(callback) {
    this._checkPermission();
    this._onDisabled = callback;
  },

  set onfrequencychange(callback) {
    this._checkPermission();
    this._onFrequencyChange = callback;
  },

  disable: function nsIDOMFMRadio_disable() {
    this._checkPermission();
    return this._call("disable", null);
  },

  enable: function nsIDOMFMRadio_enable(frequency) {
    this._checkPermission();
    return this._call("enable", frequency);
  },

  setFrequency: function nsIDOMFMRadio_setFreq(frequency) {
    this._checkPermission();
    return this._call("setFrequency", frequency);
  },

  seekDown: function nsIDOMFMRadio_seekDown() {
    this._checkPermission();
    return this._call("seekDown", null);
  },

  seekUp: function nsIDOMFMRadio_seekUp() {
    this._checkPermission();
    return this._call("seekUp", null);
  },

  cancelSeek: function nsIDOMFMRadio_cancelSeek() {
    this._checkPermission();
    return this._call("cancelSeek", null);
  },

  // nsIJSDOMEventTarget
  addEventListener: function(type, listener, useCapture, wantsUntrusted) {
    this._checkPermission();

    if (!this._eventTypes) {
      this._eventTypes = {};
    }

    if (!listener) {
      return;
    }

    var listeners = this._eventTypes[type];
    if (!listeners) {
      listeners = this._eventTypes[type] = [];
    }

    useCapture = !!useCapture;
    for (let i = 0, len = listeners.length; i < len; i++) {
      let l = listeners[i];
      if (l && l.listener === listener && l.useCapture === useCapture) {
        return;
      }
    }

    listeners.push({
      listener: listener,
      useCapture: useCapture
    });
  },

  removeEventListener: function(type, listener, useCapture) {
    this._checkPermission();

    if (!this._eventTypes) {
      return;
    }

    useCapture = !!useCapture;

    var listeners = this._eventTypes[type];
    if (listeners) {
      for (let i = 0, len = listeners.length; i < len; i++) {
        let l = listeners[i];
        if (l && l.listener === listener && l.useCapture === useCapture) {
          listeners.splice(i, 1);
        }
      }
    }
  },

  dispatchEvent: function(evt) {
    this._checkPermission();

    if (!this._eventTypes) {
      return;
    }

    let type = evt.type;
    var listeners = this._eventTypes[type];
    if (listeners) {
      for (let i = 0, len = listeners.length; i < len; i++) {
        let listener = listeners[i].listener;

        try {
          if (typeof listener == "function") {
            listener.call(this, evt);
          } else if (listener && listener.handleEvent && typeof listener.handleEvent == "function") {
            listener.handleEvent(evt);
          }
        } catch (e) {
          debug("Exception is caught: " + e);
        }
      }
    }
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([DOMFMRadio]);

