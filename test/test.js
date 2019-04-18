"use strict";

process.on('unhandledRejection', (up) => { throw up; });

const assert = require('assert');

const canberra = require('..');

async function main() {
    const ctx = new canberra.Context();

    try {
        ctx.cache({
            [canberra.Property.EVENT_ID]: 'message-new-incoming'
        });
    } catch(e) {
        if (e.code !== canberra.Error.NOTSUPPORTED)
            throw e;
    }

    try {
        setTimeout(() => {
            assert(ctx.playing(0));
            ctx.cancel(0);
        }, 200);

        await ctx.play(0, {
            [canberra.Property.EVENT_ID]: 'audio-test-signal'
        });
    } catch(e) {
        assert.strictEqual(e.code, canberra.Error.CANCELED);
        console.error(e);
    }
    ctx.destroy();
}
main();
