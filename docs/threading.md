# Threading model

## Thread inventory

| Thread | Owner | Starts in | Responsibility |
|---|---|---|---|
| **Main thread** | `Client` / `Server` `main()` | process entry | Lifecycle, blocking poll loop (`IsRunning()`), event dispatch |
| **Render thread** | `Renderer::m_ThreadRenderer` (`std::jthread`) | `Renderer::Start()` | All OpenGL calls, GLFW polling, physics tick, input processing, GUI |
| **Network ENet thread** | `NetworkClient::m_EventThread` / `NetworkServer::m_EventThread` (`std::jthread`) | `NetworkClient/Server::Start()` | Raw ENet event loop, deserializes packets into typed messages, pushes to `m_IncomingMessages` queue |
| **Network dispatch thread** | `NetworkClient::m_DispatchIncomingMessagesThread` / `NetworkServer::m_DispatchIncomingMessagesThread` (`std::jthread`) | `NetworkClient/Server::Start()` | Drains `m_IncomingMessages`, fires `EvtMessageReceived` — **this is the thread on which message handlers run** |
| **Mesh builder pool** | `WorldRenderer` / `MeshBuilder` | on demand | Async chunk mesh generation (read-only on chunk data) |
| **World generator pool** | `WorldManager` / `WorldGenerator` | on demand | Async procedural terrain generation |
| **Timer threads** | `onion::Timer` instances | `Timer::Start()` | Periodic callbacks (player info broadcast ~50 ms, entity snapshots ~100 ms) |

## Critical rule: OpenGL is single-threaded

**All OpenGL calls must happen on the render thread.** `Renderer::RenderThreadFunction` owns the GL context. Calling any GL function from the main thread or network thread will crash or corrupt state silently.

## Cross-thread data flow (client)

```
Network dispatch thread
  └─ Client::Handle_NetworkMessageReceived()
       ├─ Handle_ChunkDataMessageReceived()   → WorldManager::AddChunk()
       ├─ Handle_BlocksChangedMessageReceived() → WorldManager::SetBlocks()
       └─ Handle_EntitySnapshotMessageReceived() → WorldManager::UpdateEntities()

Render thread
  └─ WorldRenderer reads WorldManager chunk data (mesh building reads via shared_mutex)
```

`WorldManager` mutations from the network dispatch thread and reads from the render thread are guarded by a **per-chunk `std::shared_mutex`** (`Chunk::m_Mutex`). There is no global WorldManager lock. An agent adding code that reads or writes chunk data must acquire the appropriate lock (shared for reads, exclusive for writes).

## What is safe to call from where

| Operation | Main thread | Render thread | Network dispatch thread |
|---|---|---|---|
| OpenGL calls | **NO** | yes | **NO** |
| `WorldManager::AddChunk` | yes | yes (with lock) | yes (with lock) |
| `WorldManager::SetBlocks` | yes | yes (with lock) | yes (with lock) |
| `Renderer::SetRenderState` | yes (`m_MutexRenderState` guards it) | yes | no |
| Fire `onion::Event<T>` | yes | yes | yes — but handlers run on the caller's thread |
| `NetworkClient::Send` | yes | yes | yes (uses `m_OutgoingMessages` queue internally) |

## `onion::Event<T>` threading note

Event handlers run synchronously on the thread that fires the event. `EvtMessageReceived` is fired by the network dispatch thread — so `Client::Handle_*` methods all execute on the network dispatch thread, not the main thread. Do not call OpenGL from any event handler subscribed to network events.
