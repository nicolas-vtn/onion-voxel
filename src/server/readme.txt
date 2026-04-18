Information about the server:

You can edit the "config_server.json" to configure the server.

HELP :

ServerName : The name of the server, displayed in the multiplayer menu.
UUID : The unique identifier of the server, used for multiplayer connection. You can generate a new one with an online UUID generator if you want to create a new server.
Port : The port used for multiplayer connection.
Seed : The seed used for world generation. You can use any unsigned integer value. [0, 4 294 967 295]
SimulationDistance : The simulation distance used for the server. It determines how many chunks around the player are loaded and simulated.
WorldGenerationType : The world generation type used for the server.
                        - 0 : Demo Blocks
                        - 1 : Superflat
                        - 2 : Classic No Biomes
                        - 3 : Classic
                        - 4 : Biome Visualizer


EXAMPLE :

{
    "ServerName": "Brand New Server !",
    "UUID": "e02f475cf9578b9290887848287658d0",
    "Port": 7777,
    "MOTD": "\u00a7oWelcome \u00a7rto the \u00a72\u00a7lOnion::Voxel\u00a74 Server !",
    "Seed": 2,
    "SimulationDistance": 4,
    "WorldGenerationType": 1
}
