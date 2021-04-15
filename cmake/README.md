# SDL2 CMake modules

This repository contains [CMake][] modules for finding and using the SDL2
library as well as other related libraries:

- [SDL2][]
- [SDL2_image][]
- [SDL2_ttf][]
- [SDL2_net][]
- [SDL2_mixer][]
- [SDL2_gfx][]

These modules are based on the SDL (1.2) modules, with the same names,
distributed with the CMake project. The SDL2_gfx module is also based
on the SDL_image module.

## Details and Improvements

The improvements made to these modules are as follows:

**FindSDL2.cmake**

- Adapt `FindSDL.cmake` to `SDL2` (`FindSDL2.cmake`).
- Add cache variables for more flexibility:<br>
    `SDL2_PATH`, `SDL2_NO_DEFAULT_PATH`
- Mark `Threads` as a required dependency for non-OSX systems.
- Modernize the `FindSDL2.cmake` module by creating specific targets:
  - `SDL2::Core` : Library project should link to `SDL2::Core`
  - `SDL2::Main` : Application project should link to `SDL2::Main`

*For more details, please see the embedded documentation in `FindSDL2.cmake` file.*

**FindSDL2_&lt;COMPONENT&gt;.cmake**

- Adapt `FindSDL_<COMPONENT>.cmake` to `SDL2_<COMPONENT>` (`FindSDL2_<COMPONENT>.cmake`).
- Add cache variables for more flexibility:<br>
    `SDL2_<COMPONENT>_PATH`, `SDL2_<COMPONENT>_NO_DEFAULT_PATH`
- Add `SDL2` as a required dependency.
- Modernize the `FindSDL2_<COMPONENT>.cmake` modules by creating specific targets:<br>
    `SDL2::Image`, `SDL2::TTF`, `SDL2::Net`, `SDL2::Mixer` and `SDL2::GFX`.

*For more details, please see the embedded documentation in
`FindSDL2_<COMPONENT>.cmake` file.*

## Usage

In order to use the SDL2 CMake modules, we have to clone this repository in a
sud-directory `cmake/sdl2` in our project as follows:

```sh
cd <PROJECT_DIR>
git clone https://gitlab.com/aminosbh/sdl2-cmake-modules.git cmake/sdl2
rm -rf cmake/sdl2/.git
```

Or if we are using git for our project, we can add this repository as a
submodule as follows:

```sh
cd <PROJECT_DIR>
git submodule add https://gitlab.com/aminosbh/sdl2-cmake-modules.git cmake/sdl2
git commit -m "Add SDL2 CMake modules"
```

Then we should specify the modules path in the main CMakeLists.txt file like
the following:

```cmake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)
```

Finally, we can use the SDL2 modules. There is two approaches that can be
adopted: A legacy approach and a modern approach. Both of them are supported.

### Modern CMake

We can link to the SDL2:: targets like the following example:<br>
*This example requires the SDL2, SDL2_image and the SDL2_gfx libraries*

```cmake
# Find SDL2, SDL2_image and SDL2_gfx libraries
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_gfx REQUIRED)

# Link SDL2::Main, SDL2::Image and SDL2::GFX to our project
target_link_libraries(${PROJECT_NAME} SDL2::Main SDL2::Image SDL2::GFX)
```

*Use the appropriate packages for you project.*<br>
*Please see above, for the whole list of packages*<br>
*For more details, please see the embedded documentation in modules files*

### Legacy CMake

We can also specify manually the include directories and libraries to link to:

```cmake
# Find and link SDL2
find_package(SDL2 REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

# Find and link SDL2_image
find_package(SDL2_image REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SDL2_IMAGE_LIBRARIES})

# Find and link SDL2_gfx
find_package(SDL2_gfx REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_GFX_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SDL2_GFX_LIBRARIES})

```

*For more details, please see the embedded documentation in modules files*

## Special customization variables

Each module have special customization cache variables that can be used to help
the modules find the appropriate libraries:

- `SDL2_PATH` and `SDL2_<COMPONENT>_PATH`:<br>
  Can be specified to set the root search path for the `SDL2` and `SDL2_<COMPONENT>`
- `SDL2_NO_DEFAULT_PATH` and `SDL2_<COMPONENT>_NO_DEFAULT_PATH`:<br>
  Disable search `SDL2/SDL2_<COMPONENT>` library in default path:<br>
    If `SDL2[_<COMPONENT>]_PATH` is set, defaults to ON<br>
    Else defaults to OFF
- `SDL2_INCLUDE_DIR` and `SDL2_<COMPONENT>_INCLUDE_DIR`:<br>
  Set headers path. (Override)
- `SDL2_LIBRARY` and `SDL2_<COMPONENT>_LIBRARY`:<br>
  Set the library (.dll, .so, .a, etc) path. (Override)
- `SDL2MAIN_LIBRAY`:<br>
  Set the `SDL2main` library (.a) path. (Override)

These variables could be used in case of Windows projects, and when the
libraries are not localized in a standard pathes. They can be specified when
executing the `cmake` command or when using the [CMake GUI][] (They are marked
as advanced).

**cmake command example:**

```sh
mkdir build
cd build
cmake .. -DSDL2_PATH="/path/to/sdl2"
```

**CMakeLists.txt example:**

If we embed, for example, binaries of the SDL2_ttf in our project, we can
specify the cache variables values just before calling the `find_package`
command as follows:

```cmake
set(SDL2_TTF_PATH "/path/to/sdl2_ttf" CACHE BOOL "" FORCE)
find_package(SDL2_ttf REQUIRED)
```

## License

Maintainer: Amine B. Hassouna [@aminosbh](https://gitlab.com/aminosbh)

The SDL2 CMake modules are based on the SDL (1.2) modules available with the
CMake project which is distributed under the OSI-approved BSD 3-Clause License.

The SDL2 CMake modules are also distributed under the OSI-approved BSD
3-Clause License. See accompanying file [Copyright.txt](Copyright.txt).



[CMake]: https://cmake.org
[CMake GUI]: https://cmake.org/runningcmake
[SDL2]: https://www.libsdl.org
[SDL2_image]: https://www.libsdl.org/projects/SDL_image
[SDL2_ttf]: https://www.libsdl.org/projects/SDL_ttf
[SDL2_net]: https://www.libsdl.org/projects/SDL_net
[SDL2_mixer]: https://www.libsdl.org/projects/SDL_mixer
[SDL2_gfx]: http://www.ferzkopp.net/wordpress/2016/01/02/sdl_gfx-sdl2_gfx
