# Node.js bindings for libcanberra

[libcanberra](http://0pointer.de/lennart/projects/libcanberra/) is a library to play
event sounds on Linux desktops.

This repository hosts the node.js bindings to it, which allows using libcanberra from
node.js applications (such as electron or node-webkit apps, but also plain node).

## Usage

This package contains tiny, idiomatic JavaScript bindings. For the details of each
API, see the [libcanberra documentation](http://0pointer.de/lennart/projects/libcanberra/#documentation).

### Creating a context

```
const canberra = require('canberra')

const ctx = new canberra.Context({
    [canberra.Property.APPLICATION_NAME]: 'my test app',
    [canberra.Property.APPLICATION_VERSION]: '1.0.0',
});
```

Properties passed to the context constructor are optional. They are used to match
a sound to an application, for example to allow the user to disable all sounds from
a certain app.

### Playing a sound

```
ctx.play(0, {
    [canberra.Property.EVENT_ID]: 'bell'
});
```

The first argument is a numeric ID of your choice. You can pass the same ID to
`ctx.playing(id)` to check if the sound is still playing or not, and to `ctx.cancel(id)`
to stop playing. If you reuse the ID, a call to `ctx.cancel(id)` will cancel all instances.

The second argument describes what sound to play, as [libcanberra properties](http://0pointer.de/lennart/projects/libcanberra/gtkdoc/libcanberra-canberra.html#CA-PROP-MEDIA-NAME:CAPS). Most likely, you will want to set `canberra.Property.EVENT_ID` to the name
of a sound in the [sound theme](https://www.freedesktop.org/wiki/Specifications/sound-theme-spec/).

The `play()` API returns a promise that will be fulfilled when the sound has completed playing.

### Caching

```
ctx.cache({
    [canberra.Property.EVENT_ID]: 'bell'
});
```

To reduce latency caused by disk access and decompression, you can cache sounds in the sound server
(Pulseaudio, usually). The arguments are the same as `play()`.

### Error handling

All APIs except `play()` are synchronous, and will throw an error on failure (such as file not found,
or Pulseaudio server not available). The error will have the `code` property set to the libcanberra
error code, which is exposed as `canberra.Error` by this package.

`play()` is asynchronous and will return a promise; the promise will be rejected with the same
error format on failure.

### Cleanup

```
ctx.destroy()
```

`ctx.destroy()` MUST be called to cleanup resources when done. Otherwise the node.js process
will not terminate, and memory or file descriptors will be leaked.
