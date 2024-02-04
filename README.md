# mini-minecraft-mini-mojang
#### Group: Mini Mojang
#### Group members: Shuo Sun, Zhenzhong Tang, Jianping Li

## milestone 1 Features

### Procedural generation of terrain using noise functions
Implemented by: Shuo Sun
- Implemented several noise functions to generate the terrain procedurally, including Perlin, Worley, and FBM.
- Using Perlin Noise with FBM to generate the height of the mountain and the grassland biome.
- Using a Perlin noise with a smoothstep function to interpolate between the two biomes.
- Assign block type according to the generated height.
- Implemented the feature to expand chunks when moving through the border of a zone.

### Altering the provided Terrain system to efficiently store and render blocks
Implemented by: Zhenzhong Tang
- Only create VBO data for block faces that lie on the boundary between an EMPTY block and a filled block.
- Cache the VBO data for each chunk and reuse it when rendering the same chunk.
- Cached VBO is invalidated when `Chunk::setBlockAt` is called.

### Constructing a controllable Player class with simple physics
Implemented by: Jianping Li
- Implement for flight mode and non-flight Mode. Support key control with WASDQEF and mouse
- Support collision detection when in non-flight Mode,using grid matching
- Mimic the real world with friction and gravity

## milestone 2 Features

### Texturing and Texture Animation
Implemented by: Zhenzhong Tang
- Implemented texture and normal mapping.
- Support texture animation of WATER and LAVA by changing the texture coordinates of the block faces.
- Correct opaque blocks and transparent blocks rendering order.

### cave systems
implemented by: shuo sun
- sampled 3d perlin noise at each block under the 128th layer to determine whether it is in a cave
- tuned noise threshold to make natural and reasonable caves
- make empty blocks under a certain layer of lava
- set up framebuffer and shader programs for posteffects
- programmed post shaders to make visual effects for under water and under lava

### Multi-threading
Implemented by: Jianping Li
- terrain expansion, seeing surrounding 5x5 terrain zones to check if they have set up blocks.
- destory the VBO Data when moving far enough, But the Chunk data is kept,destroy all VBO data when walk too far away for efficiency.
- For the current terrian zone, if no Chunk then generated it If has Chunk but no VBOData inside, fill the VBOData
- last step to fill data and send to GPU
- difficulties: design and code


## milestone 3 Features

### Additional Biomes
Implemented by: Shuo Sun
- Add additional biomes like SNOWPEAK, MUSHLAND, DARKFOREST, OAKFOREST, DESERT
- Using SimplexNoise instead of PerlinNoise to generate weirdness and humidity, and generate height by these two factors
- The biome type is decided by humidity and height, so that each border transition is smooth
- Based on biome type, procedurally place assets additional assets based using probability
- To make biomes and terrains more reasonable, used some rules to make DESERT, and MUSLAND only appear on low height

### Procedurally placed assets
Implemented by: Shuo Sun
- Add additional blocks like CACTUS, MUSHROOM, TREE, LEAF, and other plants
- The structure of the tree and mushroom are pre-defined and procedurally generated on each block
- Different assets are placed on different biome based on different density by using probability
- Different types of trees are generated based on height, birch trees only appear on high altitude

### Skybox, Day and Night Cycle
Implemented by: Zhenzhong Tang
- Implemented a skybox with a day and night cycle, varing lambert shading (light position and hue) based on the sun's position.
- Clouds are randomly generated using a Worley noise displacement of hardcoded color palette.
- Smoothened the transition between day and night.
- Adjusted cornoa coloring order to correctly blend sky color transition.


### Inventory and onscreen GUI
Implemented by: Jianping Li
- Implemented UI, showing block type and the number of each block
- User can select which type of block they will place by pressing a particular key I fot the UI
- Use left button of mouse to break the block, that block's number will increase by 1.
- Use right button of mouse to place the block, that block's number will decrease by 1.

Difficulties:
Set up signal for different button and update the number of blocks



