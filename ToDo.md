# To Do

## Proper Gui Class
Create a proper Gui class to handle all the UI elements and interactions

- Main Menu (Singleplayer, Multiplayer, Options, Quit)
- Singleplayer Menu (Placeholder, only Back button)
- Multiplayer Menu (Placeholder, only Back button)
- Dev Menu (Dev / Test menu, with Back button)
- Options Menu (Ressource Packs, Back button)
- Background
- Uniform Naming (Load / Initialize / ...)
- Implement a Ressource Manager to handle loading and unloading of textures, and passing the ressource manager to UI elements

 
## Sounds
Add sound effects and background music.

- Sound Effect Button Click  
- Background Music


## Textures
- Hot texture reloading
- Texturepack support (Menu and selection)
 

## World

### World Manager
- Server : Handle loaded chunks, generate new chunks, unload chunks, ...
- Client : Hanle loaded chunks, request new chunks, unload chunks, partial render distance, ...
 
### Add a Free Camera
- Use Camera from VoxelEngine project

### Add a basic world renderer
- Textures from Minecraft Texturepack
- Block Rotation
- Mesh Builder
- Chunk Rendering

### Add ImGui debug pannel
- Create a Debug pannel with basic info

## Networking

### Client - Server dialog (Tcp or Udp ?)
- Connection / Disconnection handling
- Handsake (Player Info, Server Info, World Info, ...)
- Request / Send / Receive Chunks
- Send / Recieve Player states

## Server Logic
- Physics (Gravity, Collision, ...)
- Chunk Management (Generate, Load, Unload, ...)
- Player Management (Join, Leave, State Update, ...)
- 

## World Generation

### Generate a Chunk
- Gen Chunk on Server (Seed Based, configurable world parameters)
- Stream it to client
- Block ID / Block States

## Misc

- Add a proper Logger
- Change Mouse Cursor (Crosshair, Hand, ...)
