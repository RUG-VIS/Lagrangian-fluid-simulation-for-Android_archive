## To discuss in thesis
- Instanced rendering not beneficial for current setup
- research objective and res. question should be in intro; Make a contribution subsection.
- Describe new approach and how to design experiments (methods section) -> explain  how testing what I did answers R.Q.
- discussion -> discuss exp., impact, (loading different data, features, interactions, what can be done), future work (tried out things, bottlenecks, load distrib.)
- Describe required dataset(s) either before methods or very first in methods
- Reuse abstract and intro from proposal
- background section can be based on state-of-the-art but needs updating to be relevant
- Discuss capped frame-rate due to v-sync and its impact on the measurements in discussion sections

## Android side
- Delete unused project files / folders

## Native side
- Create a common super class for timers

## Current bottlenecks
For a lot of particles, or randomly distributed particles in a dense grid, the bottleneck is
the grid interpolation, as a lot of different vertices are being accessed, causing cache misses
and lowering throughput. 
The compute shaders (best) approach might benefit from a dynamic
grid-wise parallelization -> Not necessarily.
-> Could simply due to the fact that the particles in thread-groups are far apart in memory -> No simple fix
- -> Read about warps and global memory
- -> Discuss the CPU equivalent
- Java side handling of opengl surface and egl context


## Stuff to research
- Try cuberiles after all
- Direct volume rendering / volume ray casting
- The outline box approach

## General project
- Sort out references from used code
- Make sure to use same case (e.g. camelCase)
- Try better rendering methods when exatra time, e.g. thesis draft is being review ->volume rendering / volume ray casting, find new ones
- Mention the different (attempted) different ways of rendering stuff - so far: LIC
- Make a measurement for the "new bottleneck" see how the performance changes with a more random distrib. (either perlin, or bigger double-gyre)
- Make proper APKs for a) reduced, b) full version of the app
- Make measurements for (very) small numbers of particles -> see if v-sync doesn't make it pointless
- Question: Since a lot of the app is asynchronous, for compute shaders especially cause lot of GPU time, is it okay to just limit (unrelated) background processes and measure the wall-clock time?
  - Ideally measure it all separately, and report that way.
  - Measure, GPU compute, GPU render, GPU buffer load, CPU file load, CPU time ... all separately, as any could potentially become bottleneck
- Update main readme with gradlew commands

## Next meeting points

