Vulkan renderer working in progress, with mesh shading and ray tracing written in C23 with minimal library dependencies (cgltf, volk, stb image) written from scratch for Win32.

Orbit camera usage: left-click to orbit eye, right-click to pan, wheel to zoom in and out.

Simple diffuse lighting:

<img width="1920" height="1035" alt="{F75FAFFD-9617-40D1-B74B-6588B3DCB3A9}" src="https://github.com/user-attachments/assets/b191838d-1eec-4f65-86e6-6c830ff1995a" />

Ray tracing with queries with shadow rays:

<img width="1920" height="1025" alt="{79F038C0-B69F-4DAE-AB4D-452F3BC86DA5}" src="https://github.com/user-attachments/assets/a6885e5f-70c8-4275-8309-5ca460f79e8a" />

Colors highlighting individual meshlets:

<img width="1920" height="1032" alt="{2C9913E1-49C9-47D6-93AF-40BEE56DDC8F}" src="https://github.com/user-attachments/assets/bb3df0fa-2574-41c4-87b7-dbe37a2ebf48" />

Normals:

<img width="1917" height="1033" alt="{A5DF8784-C76B-4A40-A0E6-FF53A5541C35}" src="https://github.com/user-attachments/assets/393d9412-f971-4ee0-974c-c931eac613ac" />

Messing around with water mesh shader:

<img width="1920" height="1080" alt="{66524D83-8A9D-463E-AAE6-1EB045E62C76}" src="https://github.com/user-attachments/assets/420a3a07-78bd-41af-a2c2-24c6f9603fff" />


For command line git init, code and shader build, follow these three steps:

0. run 'git submodule update --init' 
1. run 'build_all.bat'
2. run 'build\vulkan_3d_release.exe'

For msvc build, open the project under win32-solution.

Tested on NVIDIA and AMD vendors.

TODO: Support for other platforms (linux, macos, ios), proper lighting models etc.
