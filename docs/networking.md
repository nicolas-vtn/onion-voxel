# Networking

## Stack

ENet over UDP. Binary serialization via Cereal (`BinaryInputArchive` / `BinaryOutputArchive`).

## Packet framing

Every packet is a Cereal binary stream with two sections serialized back-to-back:

```
[ MessageHeader (5 bytes) ][ Message payload ]
  uint8_t  Type
  uint32_t ClientHandle
```

`MessageHeader` is always read first. Its `Type` field drives the `DeserializeMessage` switch to produce the correctly-typed `NetworkMessage` variant. `ClientHandle` is the server-assigned peer ID (not a player UUID) — the server stamps it on outbound messages so the client knows its own handle; clients stamp it on outbound messages so the server knows the sender.

## Message type catalogue

Defined in `src/shared/shared/network_messages/NetworkMessages.hpp`:

```cpp
using NetworkMessage = std::variant<
    ClientInfoMsg,      // client → server: player name + UUID on connect
    ServerInfoMsg,      // server → client: assigned ClientHandle + server name
    ChunkDataMsg,       // server → client: full serialized chunk
    PlayerInfoMsg,      // client → server: position + rotation tick
    BlocksChangedMsg,   // bidirectional: list of block changes
    RequestChunksMsg,   // client → server: list of chunk positions needed
    EntitySnapshotMsg,  // server → client: all entity transforms
    ServerMotdMsg,      // server → client: MOTD data
    RequestMotdMsg      // client → server: request MOTD
>;
```

Each message type is a simple struct with a Cereal `serialize` template in its own subdirectory under `src/shared/shared/network_messages/`.

## How to add a new message type

Touch these five places in order:

1. **Create the struct** — add `src/shared/shared/network_messages/<name>_msg/<NameMsg>.hpp` with a `serialize` template:
   ```cpp
   template <class Archive> void serialize(Archive& ar) { ar(Field1, Field2); }
   ```

2. **Add the enum value** — `MessageHeader::eType` in `src/shared/shared/network_messages/MessageHeader.hpp`:
   ```cpp
   enum class eType : uint8_t { ..., YourNewType };
   ```

3. **Add to the variant** — `NetworkMessages.hpp`, both the `#include` and the `std::variant` list.

4. **Add a case to `DeserializeMessage`** — in `NetworkMessages.hpp`:
   ```cpp
   case MessageHeader::eType::YourNewType: { YourNewMsg msg; archive(msg); return msg; }
   ```

5. **Handle it** — subscribe to `EvtMessageReceived` (or `EvtMessageReceived` on `NetworkServer`) and add a `std::visit` or `if (auto* m = std::get_if<YourNewMsg>(&message))` branch in the appropriate `Handle_NetworkMessageReceived` on the client (`src/client/src/Client.cpp`) or server (`src/server/src/Server.cpp`).

## Sending a message

```cpp
// Client side
m_NetworkClient.Send(YourNewMsg{...});           // reliable by default
m_NetworkClient.Send(YourNewMsg{...}, false);    // unreliable

// Server side
m_NetworkServer.Send(clientHandle, YourNewMsg{...});
m_NetworkServer.Broadcast(YourNewMsg{...});
```

`Send` is thread-safe — it pushes to an internal `ThreadSafeQueue<OutgoingMessage>` which the ENet thread drains.

## Threading note

Message handlers (`Handle_NetworkMessageReceived` and all its callees) run on the **network dispatch thread**, not the main thread. See `docs/threading.md` for the full cross-thread safety rules.
