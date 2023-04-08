'use strict';

const internal = require('bindings')('nostr-pow');
const { createHash } = require('crypto');

function computeSync(event, targetDifficulty) {
    const [prefix, suffix] = prepare(event, targetDifficulty);
    const nonce = internal.compute(prefix, suffix, targetDifficulty);
    return finish(event, nonce);
}

function computeAsync(event, targetDifficulty) {
    return new Promise((fulfill, reject) => {
        try {
            const [prefix, suffix] = prepare(event, targetDifficulty);
            internal.compute(prefix, suffix, targetDifficulty, (err, nonce) => {
                if (err) {
                    reject(err);
                } else {
                    fulfill(finish(event, nonce));
                }
            });
        } catch (err) {
            reject(err);
        }
    });
}

function prepare(event, targetDifficulty) {
    if (!(event instanceof Object) ||
        typeof targetDifficulty !== 'number') {
        throw new Error('Invalid event or targetDifficulty');
    }
    if (targetDifficulty < 0) {
        throw new Error('targetDifficulty cannot be negative');
    }
    if (targetDifficulty > 64) {
        throw new Error('targetDifficulty cannot exceed 64');
    }

    event.tags = event.tags.filter(tag => tag[0] !== 'nonce');

    let separator;
    const serialized = serializeEvent(event);
    for (let i = 0;; ++i) {
        separator = `<<${i}>>`;
        if (serialized.indexOf(separator) === -1) break;
    }

    event.tags.push(['nonce', separator, targetDifficulty.toString()]);

    const [prefix, suffix] = serializeEvent(event).split(separator);
    return [Buffer.from(prefix, 'utf8'), Buffer.from(suffix, 'utf8')];
}

function finish(event, nonce) {
    event.tags[event.tags.length - 1][1] = nonce.toString();
    event.id = createHash('sha256').update(serializeEvent(event)).digest('hex');
    return event;
}

function serializeEvent(event) {
    return JSON.stringify([
        0,
        event.pubkey,
        event.created_at,
        event.kind,
        event.tags,
        event.content
    ]);
}

module.exports = { computeSync, computeAsync };
