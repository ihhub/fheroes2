# makefile
# project: Free Heroes2
#
# Options:
# DEBUG: build in debug mode
# RELEASE: build with addons extensions
#
# WITHOUT_ZLIB: build without zlib (disable comressed save files)
# WITHOUT_MIXER: build without SDL_mixer library
# WITHOUT_AUDIOCD: disable audio cd support
# WITHOUT_UNICODE: build without unicode (disable translation and ttf font)
# WITHOUT_IMAGE: build without SDL_image library (disable cache image, icn2png)
# WITHOUT_XML: skip build tinyxml, used for load alt. resources
# WITH_TOOLS: build tools
# WITH_AI: empty (default), simple (Non-commercial License)
# 
# -DCONFIGURE_FHEROES2_LOCALEDIR: system locale dir
# -DCONFIGURE_FHEROES2_DATA: system fheroes2 game dir
#

TARGET	:= fheroes2
MAKE	:= make

all:
	$(MAKE) -C src WITH_AI=empty
	@cp src/dist/$(TARGET) .

clean:
	$(MAKE) -C src clean
