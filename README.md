# Mini Minecraft

## Introduction

This project implements a Qt-based C++ application that renders an interactive Minecraft-style world using OpenGL. The player can explore the environment in a free-flight mode or navigate with a physics-based system that simulates collisions, gravity, friction, and buoyancy. Realism is enhanced through procedurally generated terrain with mixed biomes, soft shadows, screen-space water reflections and refractions, atmospheric scattering, etc. The application runs in real-time on desktop or laptop systems equipped with a high-end GPU.

Here is a demo video about this application [on YouTube](https://www.youtube.com/watch?v=PiAYUEdfHoc).

[![Video cover](./images/video_cover.jpg)](https://www.youtube.com/watch?v=PiAYUEdfHoc)

## Features

### Procedural Terrain Generation

- Each biome (plains, grasslands, mountains) is generated by sampling high-frequency Perlin or Worley noise. A low-frequency Perlin noise determines the biome map, which is used to interpolate between biome-specific terrain.
- Rivers are carved using low-frequency, thresholded Perlin noise. Irregular banks are created by perturbing sampling coordinates with high-frequency noise.
- Caves are generated using 3D Perlin noise.
- Water and lava levels are fixed constants.

### Player Physics

- Collision detection uses swept AABB between the player's bounding box and solid terrain blocks.
- Only translational movement is considered; orientation is controlled manually via arrow keys.
- Gravity is always active. In water or lava, gravity is scaled down to simulate buoyancy.
- Medium-dependent drag applies exponential speed decay whether in air, water, or lava.

### Normal Mapping

If a normal map is available in the texture asset, it is used for lighting; otherwise, face normals are applied.

### Multithreaded Terrain Processing

- Terrain generation runs on worker threads. Initial vertex attributes are prepared concurrently but uploaded on the main thread due to OpenGL constraints.
- Vertex attribute regeneration is also threaded when blocks change or chunk visibility updates.
- Each chunk tracks three version IDs:
  - **Block Version**: Actual block data.
  - **Attribute Version**: Triggers regeneration if outdated.
  - **GPU Version**: Triggers GPU upload if outdated.

### Water Surface Waves

- Water surface is defined by a sum of sinusoidal wave functions, each exponentiated for sharper peaks.
- At runtime, each water block is rendered as a quad. Smooth surface normals are computed per-fragment.

### Deferred Shading Pipeline

A deferred rendering pipeline enables complex visual effects across multiple passes:

- **Shadow Mapping Passes**: Record linear depth and squared depth for cascaded shadow maps.
- **Geometry Passes**:
  - **Main Camera (Solid)**: Capture depth, normal, albedo, block type, and medium type.
  - **Main Camera (Translucent)**: Same as above, applied only to water.
  - **Reflected View**: Records geometry from a mirrored camera with a widened field of view. Used for screen-space reflection ray marching.
  - **Refracted View**: Records geometry from a camera positioned at a shifted height relative to the water surface, with an expanded field of view. Used for refraction ray marching.
- **Lighting Pass**: Composites all information and renders to the screen.

### Screen-Space Water Reflections and Refractions

For each water fragment, reflection and refraction effects are computed by ray marching in screen space along directions derived from Snell’s Law and modulated by Fresnel reflectance. The ray continues until it intersects a terrain block, where lighting calculations are performed.

Ray marching directly in the main camera’s screen space is often ineffective, as rays frequently exit the view frustum before hitting any geometry. To keep more rays within bounds, auxiliary reflected and refracted views are rendered during the geometry passes and used to provide more effective G-buffer samples for the ray marching process.

### Cascaded Shadow Mapping

To render terrain shadows cast by sunlight, the main camera's view frustum is split into six logarithmically spaced ranges—using finer granularity near the viewer and coarser farther away. Each range is rendered using orthographic projection to produce a cascade of shadow maps.

Each cascade stores a 32-bit floating-point **linear depth buffer**, representing the distance from the light's projection plane to the nearest occluder. This improves depth precision and reduces common artifacts like shadow acne and Peter Panning.

### Soft Shadows via Variance Shadow Mapping

To support soft shadows, each shadow map additionally stores the **squared depth**, enabling estimation of depth variance. For a given fragment, the **shadow softness** is computed based on the difference between its depth and the average shadow depth in the filter region.

Poisson disk sampling is used to blur the depth maps, and the occlusion probability is derived from the mean and variance using the Chebyshev inequality. This probabilistic shadow test modulates the final shadow intensity, producing soft edges and penumbrae.

### Simulated Atmospheric Scattering

- Approximate Rayleigh and Mie scattering are used to simulate realistic sky colors.
- Distant objects are dimmed due to out-scattering; objects aligned with the sun appear brighter from in-scattering.

### Color Space and Tone Mapping

- Texture assets are stored in sRGB and converted to linear space for lighting calculations.
- Final output is tone-mapped using the ACES curve, then converted back to sRGB for display.
