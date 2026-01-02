Vulkan renderer working in progress, with mesh shading and ray tracing written in C23 with minimal library dependencies (cgltf, volk, stb image) written from scratch for Win32.

Orbit camera usage: left-click to orbit eye, right-click to pan, wheel to zoom in and out.

Diffuse lighting model with specular highlights:

<img width="1920" height="1080" alt="{E08F3D14-795F-4011-B12F-1B874DB8DED4}" src="https://github.com/user-attachments/assets/b7b8c5eb-e465-40ce-a1a9-54ae8bcb7e56" />

Ray tracing with queries with shadow rays:

<img width="1920" height="1080" alt="{71C7E331-8EDB-41EF-B685-0E947A4A56FA}" src="https://github.com/user-attachments/assets/89b3a9aa-0a9b-4dab-a4e4-582b0453e97b" />

Messing around with super trivial water mesh shader:

<img width="1920" height="1080" alt="{75277977-D5D9-49F4-85B6-6D35A74BA9B7}" src="https://github.com/user-attachments/assets/abc8f7f1-90af-4b0b-9e6f-66ea0000efcb" />


For command line git init, code and shader build, follow these three steps:

0. run 'git submodule update --init' 
1. run 'build_all.bat'
2. run 'build\vulkan_3d_release.exe'

For msvc build, open the project under win32-solution.

Tested on NVIDIA and AMD vendors.

TODO: Support for other platforms (linux, macos, ios), proper lighting models etc.
