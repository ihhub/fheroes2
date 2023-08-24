<h1 align="center">
  <br>
  <img src="https://i.imgur.com/UQ4DFMq.png" alt="sdl2-logo" width="200"></a>
  <br>
    CMake Modern FindSDL2
  <br>
</h1>

<p align=center>
  <b> CMake modules to find SDL2 and its extensions. </b>
</p>

<p align="center">
  <a href="#usage">Usage</a> •
  <a href="#license">License</a> •
  <a href="#acknowledgments">Acknowledgments</a>
</p>

## Usage
Add this repository as a submodule to your project.
```cmake
git submodule add https://github.com/opeik/cmake-modern-findsdl2 cmake/sdl2
```
Include the new modules.
```cmake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)
```
Find the package you want to use.
```cmake
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_gfx REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_net REQUIRED)
```
Link against it.
```cmake
target_link_libraries(${PROJECT_NAME} PUBLIC SDL2::SDL2 SDL2::TTF
    SDL2::Image SDL2::Mixer SDL2::Net SDL2::Gfx)
```

If you're writing an application, link against `SDL2::SDL2`. If you're writing
a library, link against `SDL2::Core` instead.

## License
This project is licensed under the ISC license. Please see the [`LICENSE.md`](LICENSE.md)
file for details.

## Acknowledgments
* Thanks to [aminosbh](https://github.com/aminosbh) for creating similar cmake
  scripts which I referenced
