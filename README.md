# Volumetric cloud rendering
I tested the effect and performance of 5 methods for rendering the self-occlusion effect of volumetric clouds:\
i) secondary ray marching; \
ii) Exponential shadow map (ESM); \
iii) Beer shadow map (BSM);\
iv) Fourier opacity map (FOM);\
v) secondary ray marching with large amount of steps (ground truth).

This project is not built in commercial game engine but written in C++ and OpenGL.

## Visual results of the five methods
<img src="https://github.com/JingWang-IRC/VolumetricCloud/blob/main/ExtraMaterial/VisualResult.png" width="600">

## Demo videos (using secondary ray marching)
https://github.com/JingWang-IRC/VolumetricCloud/assets/75773315/b64fbcdf-5fd3-4825-a3b0-da136b4007ea


https://github.com/JingWang-IRC/VolumetricCloud/assets/75773315/c38f30d7-a7a7-46e1-a99a-23f1e94cd909


