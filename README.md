nostr-pow
=========

Fast proof-of-work nonce generator for Nostr events written for Node.

## Usage

```javascript

const { computeAsync } = require('nostr-pow');

const event = {
    pubkey: '489ac583fc30cfbee0095dd736ec46468faa8b187e311fda6269c4e18284ed0c',
    kind: 1,
    created_at: 1680947398,
    content: 'Hello, world!',
    tags: []
};

computeAsync(event, 25).then(event => {
    console.log(event);
});
```

Prints out:
```javascript
{
    pubkey: '489ac583fc30cfbee0095dd736ec46468faa8b187e311fda6269c4e18284ed0c',
    kind: 1,
    created_at: 1680947398,
    content: 'Hello, world!',
    tags: [ [ 'nonce', '2963377', '25' ] ],
    id: '0000003af010369da0aaacb0180401fd6d88f5dd0510ebbdfe0da754ecc08e31'
}
```

Once completed you can sign the event and publish it.

## Usage (synchronous)

Identical function `computeSync(event, nonce)` exists that will block the main thread and
directly return the resulting event.

## Performance

`nostr-pow` computes the hashes on the CPU using the maximum amount of hardware threads
available. See [`compute_nonce.c`](src/compute_nonce.c) for the main computation function.
