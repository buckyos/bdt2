const EventEmitter = require('events').EventEmitter;
const _ = require('underscore');

function fireError(err) {
    let shouldCatch = false;

    if (_.isFunction(this.emitter.listenerCount) && this.eventEmitter.listenerCount('error') > 0) {
        shouldCatch = true;
    }

    if (shouldCatch) {
        this.eventEmitter.catch(() => {

        });
    }

    setImmediate(() => {
        this.reject(err);
    });

    if (_.isFunction(this.emitter.listenerCount) && this.eventEmitter.listenerCount('error') > 0) {
        setImmediate(()=> {
            this.eventEmitter.emit('error', err);
            this.eventEmitter.removeAllListeners();
        });
    }

    return this.eventEmitter;
}

function promiseEvent(justPromise) {
    let resolve, reject;

    const eventEmitter = new Promise((...args) => {
        resolve = args[0];
        reject = args[1];
    });

    const promEvent = {
        resolve: resolve,
        reject: reject,
        eventEmitter: eventEmitter,
    };

    promEvent.fireError = fireError.bind(promEvent);

    if (justPromise) {
        return promEvent;
    }

    const emitter = new EventEmitter();

    // add eventEmitter to the promise
    eventEmitter._events = emitter._events;
    eventEmitter.emit = emitter.emit;
    eventEmitter.on = emitter.on;
    eventEmitter.once = emitter.once;
    eventEmitter.off = emitter.off;
    eventEmitter.listeners = emitter.listeners;
    eventEmitter.listenerCount = emitter.listenerCount;
    eventEmitter.addListener = emitter.addListener;
    eventEmitter.removeListener = emitter.removeListener;
    eventEmitter.removeAllListeners = emitter.removeAllListeners;

    return promEvent;
}


module.exports = promiseEvent;