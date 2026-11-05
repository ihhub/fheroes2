# fheroes2 lobby

A small program for network games of fheroes2. Players find a game in a list and join it without typing an address,
and games which cannot connect directly go through its relay. Anyone can run one: the game has a list of lobbies in
its settings.

## Running it

Node.js 22 or later, nothing to install:

```sh
node server.js
```

Games connect to `ws://<address>:47220/lobby`. `http://<address>:47220/` shows how many players and rooms there are.

Settings are taken from the environment:

| Variable | Default | |
| --- | --- | --- |
| `LOBBY_PORT` | `47220` | Port to listen on. |
| `LOBBY_HOST` | all addresses | Address to listen on, IPv6 and IPv4. |
| `LOBBY_NAME` | `fheroes2` | Name shown in the game, 40 characters at most. |
| `LOBBY_MESSAGE` | | Message of the day, 300 characters at most. |
| `LOBBY_RELAY` | `on` | `off` for a lobby without relay. |
| `LOBBY_PROBE` | `on` | `off` to give the addresses of hosts without checking that they can be reached. |
| `LOBBY_PRIVATE_ADDRESSES` | `off` | `on` for a lobby on a local network: private addresses are probed and given too. |
| `LOBBY_TRUST_PROXY` | `off` | `on` behind a reverse proxy: the address of a game is read from `X-Forwarded-For`. |
| `LOBBY_ROOMS_PER_ADDRESS` | `3` | Games published from one address at the same time. More for a lobby on a local network, where many players may share an address. |

## On Cloudflare

The same lobby runs on Cloudflare Workers, on the free plan (`cloudflare.js`, `wrangler.toml`). One Durable Object holds
the lobby and its relay. With an API token made from the "Edit Cloudflare Workers" template:

```sh
CLOUDFLARE_API_TOKEN=... npx wrangler deploy
```

Games then connect to `ws://lobby.<subdomain>.workers.dev`. The settings are the variables of `wrangler.toml`. The free
plan gives 100,000 requests a day (incoming WebSocket messages count one for twenty) and enough Durable Object time to
keep one lobby in memory all day long: plenty for a community of players, and a relayed game needs little.

## What it does

* Keeps the rooms published by hosts, sends the list of public rooms waiting for players to the games which ask for
  it, and gives each room an invite code (six characters, without 0, O, 1 and I).
* Checks whether each host can be reached directly: it connects to the game port of the host and sends a probe, which
  the host answers only with the key it chose when publishing. Private addresses are not probed, unless
  `LOBBY_PRIVATE_ADDRESSES` is on.
* Tells a guest how to reach the host: local addresses when both are behind the same box, then the addresses which
  answered the probe, then the relay.
* Relays the games which cannot connect directly. A host keeps a connection waiting on the relay, a guest comes with
  the ticket it got when joining, and from then on the relay forwards what one sends to the other without reading it.
* Never shows the address of a host in the list. The address goes only to the guest who joins, and not at all for
  rooms through the relay only. Nothing about players is written on disk.
* Limits connections and rooms per address, messages per second, attempts to join (invite codes cannot be guessed),
  message sizes and the bytes relayed per room and per day.

The messages are described in `protocol.js`. The game reads and writes them in
`src/fheroes2/multi/multi_lobby_protocol.cpp`.

## Files

| File | |
| --- | --- |
| `protocol.js` | Lobby messages, with the checks of every field. |
| `core.js` | Rooms, list, invite codes, probes, join plans and limits. Independent of Node.js. |
| `relay.js` | Pairs and forwards the relayed connections. Independent of Node.js. |
| `addresses.js` | IPv4 and IPv6 addresses: public or not, behind the same box or not. |
| `websocket.js` | Server side of WebSocket connections on Node.js. |
| `server.js` | The server for Node.js. |
| `cloudflare.js`, `wrangler.toml` | The lobby on Cloudflare Workers. |
| `test/` | Tests (`npm test`), and the test vectors read by the game (`node test/vectors.js --write` after changing a message). |
