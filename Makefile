# makefile
# project: Free Heroes of Might and Magic II (https://github.com/ihhub/fheroes2)
#
# Options:
# DEBUG: build in debug mode
# RELEASE: build with addons extensions
#
# WITHOUT_ZLIB: build without zlib (disable compressed save files)
# WITHOUT_MIXER: build without SDL_mixer library
# WITHOUT_AUDIOCD: disable audio CD support
# WITHOUT_UNICODE: build without unicode (disable translation and ttf font)
# WITHOUT_IMAGE: build without SDL_image library (disable cache image, icn2png)
# WITHOUT_XML: skip build tinyxml, used for load alt. resources
# WITH_TOOLS: build tools
# WITHOUT_BUNDLED_LIBS: do not build XML third party library
# FHEROES2_STRICT_COMPILATION: build with strict compilation option (makes warnings into errors)
#
# -DCONFIGURE_FHEROES2_LOCALEDIR: system locale dir
# -DCONFIGURE_FHEROES2_DATA: system fheroes2 game dir
#

TARGET	:= fheroes2

all:
	$(MAKE) -C src
	@cp src/dist/$(TARGET) .

clean:
	$(MAKE) -C src clean
	@rm -f ./$(TARGET)
