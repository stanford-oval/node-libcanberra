// This file is part of node-libcanberra
//
// Copyright (c) 2019 The Board of Trustees of The Leland Stanford Junior University
//
// Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of Stanford University nor the
//     names of its contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Giovanni Campagna <gcampagn@cs.stanford.edu>
//
"use strict";

const NativeContext = require('bindings')('canberra').Context;

const Error = {
    SUCCESS: 0,
    NOTSUPPORTED: -1,
    INVALID: -2,
    STATE: -3,
    OOM: -4,
    NODRIVER: -5,
    SYSTEM: -6,
    CORRUPT: -7,
    TOOBIG: -8,
    NOTFOUND: -9,
    DESTROYED: -10,
    CANCELED: -11,
    NOTAVAILABLE: -12,
    ACCESS: -13,
    IO: -14,
    INTERNAL: -15,
    DISABLED: -16,
    FORKED: -17,
    DISCONNECTED: -18,
};

const Property = {
    MEDIA_NAME: 'media.name',
    MEDIA_TITLE: 'media.title',
    MEDIA_ARTIST: 'media.artist',
    MEDIA_LANGUAGE: 'media.language',
    MEDIA_FILENAME: 'media.filename',
    MEDIA_ICON: 'media.icon',
    MEDIA_ICON_NAME: 'media.icon_name',
    MEDIA_ROLE: 'media.role',

    EVENT_ID: 'event.id',
    EVENT_DESCRIPTION: 'event.description',
    EVENT_MOUSE_X: 'event.mouse.x',
    EVENT_MOUSE_Y: 'event.mouse.y',
    EVENT_MOUSE_HPOS: 'event.mouse.hpos',
    EVENT_MOUSE_VPOS: 'event.mouse.vpos',
    EVENT_MOUSE_BUTTON: 'event.mouse.button',

    WINDOW_NAME: 'window.name',
    WINDOW_ID: 'window.id',
    WINDOW_ICON: 'window.icon',
    WINDOW_ICON_NAME: 'window.icon_name',
    WINDOW_X11_DISPLAY: 'window.x11.display',
    WINDOW_X11_SCREEN: 'window.x11.screen',
    WINDOW_X11_MONITOR: 'window.x11.monitor',
    WINDOW_X11_XID: 'window.x11.x11',

    APPLICATION_NAME: 'application.name',
    APPLICATION_ID: 'application.id',
    APPLICATION_VERSION: 'application.version',
    APPLICATION_ICON: 'application.icon',
    APPLICATION_ICON_NAME: 'application.icon_name',
    APPLICATION_LANGUAGE: 'application.language',
    APPLICATION_PROCESS_ID: 'application.process.id',
    APPLICATION_PROCESS_BINARY: 'application.process.binary',
    APPLICATION_PROCESS_USER: 'application.process.user',
    APPLICATION_PROCESS_HOST: 'application.process.host',

    CANBERRA_CACHE_CONTROL: 'canberra.cache.control',
    CANBERRA_VOLUME: 'canberra.volume',
    CANBERRA_XDG_THEME_NAME: 'canberra.xdg-theme.name',
    CANBERRA_XDG_THEME_OUTPUT_PROFILE: 'canberra.xdg-theme.output-profile'
};

class Context {
    constructor(props = {}) {
        this._callbacks = new Map;
        this._native = new NativeContext(props, (id, err) => {
            if (err)
                this._callbacks.get(id).reject(err);
            else
                this._callbacks.get(id).resolve();
            this._callbacks.delete(id);
        });
    }

    destroy() {
        this._native.destroy();
    }

    cancel(id) {
        this._native.cancel(id);
    }

    cache(props) {
        this._native.cache(props);
    }

    playing(id) {
        return this._native.playing(id);
    }

    play(id, props) {
        const callbacks = {};
        const promise = new Promise((resolve, reject) => {
            callbacks.resolve = resolve;
            callbacks.reject = reject;
        });
        this._callbacks.set(id, callbacks);

        this._native.play(id, props);
        return promise;
    }
}

module.exports = Context;
module.exports.Context = Context;
module.exports.Error = Error;
module.exports.Property = Property;
