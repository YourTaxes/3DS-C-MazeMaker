#Credit to all the contributers to the 3ds homebrew examples,
#This makefile is based a large amount on them, and I learned a lot from their examples


#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules
include $(TOPDIR)/version.env

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# GRAPHICS is a list of directories containing graphics files
# GFXBUILD is the directory where converted graphics files will be placed
#   If set to $(BUILD), it will statically link in the converted
#   files as if they were data files.
#
# NO_SMDH: if set to anything, no SMDH file is generated.
# ROMFS is the directory which contains the RomFS, relative to the Makefile (Optional)
# APP_TITLE is the name of the app stored in the SMDH file (Optional)
# APP_DESCRIPTION is the description of the app stored in the SMDH file (Optional)
# APP_AUTHOR is the author of the app stored in the SMDH file (Optional)
# ICON is the filename of the icon (.png), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.png
#     - icon.png
#     - <libctru folder>/default_icon.png
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
OUTDIR		:=	output
SOURCES		:=	source source/datatypes source/utils source/game_logic \
			source/states \
			source/states/Debug source/states/LevelSelect \
			source/states/MainMenu source/states/Maker \
			source/states/Maze \
			source/states/forgotten \
			source/menu_navigation
DATA		:=	data
INCLUDES	:=	source
GRAPHICS	:=	gfx
ROMFS		:=	romfs
#the t3x files go into romfs rather than being linked into the executable, so the game
#loads them at runtime with C2D_SpriteSheetLoad("romfs:/gfx/<name>.t3x")
GFXBUILD	:=	$(ROMFS)/gfx
MUSIC		:= 	music
MUSICBUILD	:= 	$(ROMFS)/music
META_DIR	:=	meta
ICON		:= 	$(META_DIR)/MenuIcon.png
APP_TITLE	:= 	Maze Maker 3DS
APP_AUTHOR	:= 	Finnegan McDevitt
APP_DESCRIPTION	:= 	Maze Maker 3DS: Make, Play, and Share Mazes

#==========================
#makerom stuff for cia file
#==========================
APP_RSF		:= 	$(META_DIR)/MazeMaker.rsf
#This is a placeholder until I finish the model
BANNER_IMAGE	:= 	$(META_DIR)/PlaceholderBanner.png
#Will add this, I am stil working on the model itself
#BANNER_CGFX	:=	$(META_DIR)/banner.cgfx
BANNER_AUDIO	:= 	$(META_DIR)/BannerAudio.ogg
#left this here in case it needs to be changed in the future
STREETPASS_ID	:= 	$(UNIQUE_ID) 
BANNER_FLAGS	:= 	visible,allow3d,extendedbanner,savedata,recordusage



#---------------------------------------------------------------------------------
# determine which args to use for bannertool, depending on if I choose to eventually do flat or 3d 
#---------------------------------------------------------------------------------
ifneq ($(strip $(BANNER_CGFX)),)
BANNER_ARG	:=	-ci "$(TOPDIR)/$(BANNER_CGFX)"
else
BANNER_ARG	:=	-i "$(TOPDIR)/$(BANNER_IMAGE)"
endif

#---------------------------------------------------------------------------------
# DEBUG=1 (or `make debug`) disables compiler optimizations and defines DEBUG.
# both of them build into the $(BUILD) directory, so they restart everything when switching modes
#---------------------------------------------------------------------------------
DEBUG		?=	0

ifeq ($(DEBUG),0)
OPTIMIZE	:=	-O2
else
OPTIMIZE	:=	-O0 -DDEBUG
endif

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall $(OPTIMIZE) -mword-relocations \
			-ffunction-sections \
			$(ARCH) -std=gnu23

CFLAGS	+=	$(INCLUDE) -D__3DS__

#add the env file's defines 
CFLAGS	+= -D_VERSION_MAJOR_=$(MAZEMAKER_VERSION_MAJOR) \
			-D_VERSION_MINOR_=$(MAZEMAKER_VERSION_MINOR) \
			-D_VERSION_MICRO_=$(MAZEMAKER_VERSION_MICRO) \
			-D_SAVEFILE_VERSION_=$(SAVEFILE_VERSION) \
			-D_STREETPASS_FILE_VERSION_=$(STREETPASS_FILE_VERSION)

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lcitro2d -lcitro3d -lctru -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CTRULIB)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(notdir $(BUILD)),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(OUTDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
SHLISTFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
GFXFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.t3s)))
MUSICFILES	:= 	$(foreach dir,$(MUSIC),$(notdir $(wildcard $(dir)/*.*)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
ifeq ($(GFXBUILD),$(BUILD))
#---------------------------------------------------------------------------------
export T3XFILES :=  $(GFXFILES:.t3s=.t3x)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
export ROMFS_T3XFILES	:=	$(patsubst %.t3s, $(GFXBUILD)/%.t3x, $(GFXFILES))
export T3XHFILES		:=	$(patsubst %.t3s, $(BUILD)/%.h, $(GFXFILES))
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_SOURCES 	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES)) \
			$(PICAFILES:.v.pica=.shbin.o) $(SHLISTFILES:.shlist=.shbin.o) \
			$(addsuffix .o,$(T3XFILES))

export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES	:=	$(PICAFILES:.v.pica=_shbin.h) $(SHLISTFILES:.shlist=_shbin.h) \
			$(addsuffix .h,$(subst .,_,$(BINFILES))) \
			$(GFXFILES:.t3s=.h)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export _3DSXDEPS	:=	$(if $(NO_SMDH),,$(OUTPUT).smdh)

#Assign the tools 
BANNERTOOL		?=	bannertool
FFMPEG			?=	ffmpeg
PYTHON			?=	python3

MAKEROM 		?=	makerom

MAKEROM_ARGS	:= 	-elf $(OUTPUT).elf -rsf $(APP_RSF) \
					-major $(MAZEMAKER_VERSION_MAJOR) -minor $(MAZEMAKER_VERSION_MINOR) -micro $(MAZEMAKER_VERSION_MICRO) \
					-icon "$(OUTPUT).smdh" -banner "$(BUILD)/banner.bnr" \
					-DUNIQUE_ID=$(UNIQUE_ID) -DAPP_TITLE="$(TITLE)"
#later: -logo meta/logo.bcma.lz -content "$(BUILD)/manual.cfa:1:1"

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.png)
	ifneq (,$(findstring $(TARGET).png,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).png
	else
		ifneq (,$(findstring icon.png,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.png
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(OUTPUT).smdh
endif

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: all clean debug checkmode cia bannerAudio

#---------------------------------------------------------------------------------
all: $(BUILD) $(OUTDIR) checkmode $(GFXBUILD) $(MUSICBUILD) $(DEPSDIR) $(ROMFS_T3XFILES) $(T3XHFILES) 
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
# unoptimized build (-O0), easier to step through in a debugger
#---------------------------------------------------------------------------------
debug:
	@$(MAKE) --no-print-directory DEBUG=1 all

#---------------------------------------------------------------------------------
# drop the objects if the last build used the other DEBUG setting, so everything
# gets recompiled with the new flags instead of mixing the two
#---------------------------------------------------------------------------------
checkmode: $(BUILD)
	@if [ "$$(cat $(BUILD)/.buildmode 2>/dev/null)" != "$(DEBUG)" ]; then \
		echo "DEBUG=$(DEBUG), rebuilding ..."; \
		rm -f $(BUILD)/*.o $(BUILD)/*.d $(OUTPUT).elf; \
		echo $(DEBUG) > $(BUILD)/.buildmode; \
	fi

#
# makes the project into a cia file
# does everything in all first 
#

cia: all bannerAudio
	@echo "Packaging $(TARGET).cia..."
#check that the rsf actually exists
	@if [ ! -f $(APP_RSF) ]; then \
		echo "Error: $(APP_RSF) not found in the project directory. Please create an RSF file first."; \
		exit 1; \
	fi
	@$(BANNERTOOL) makebanner $(BANNER_ARG) -a "$(TOPDIR)/$(BUILD)/banner.wav" -o "$(BUILD)/banner.bnr"
	@$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i "$(TOPDIR)/$(ICON)" -f $(BANNER_FLAGS) -o "$(BUILD)/icon.icn"
	echo "Starting makerom packing..."
	@$(MAKEROM) -f cia -o "$(OUTPUT).cia" -target t -exefslogo $(MAKEROM_ARGS)




# 	makerom -f cia -o $(OUTPUT).cia \
# 	-elf $(OUTPUT).elf \
# 	-rsf $(APP_RSF) \
# 	-target t \
# 	-icon $(OUTPUT).smdh \
# 	-major $(MAZEMAKER_VERSION_MAJOR) \
# 	-minor $(MAZEMAKER_VERSION_MINOR) \
# 	-micro $(MAZEMAKER_VERSION_MICRO) \
# 	-DUNIQUE_ID=$(UNIQUE_ID) \
# 	-DAPP_TITLE="$(TITLE)" \
# 	-DAPP_ROMFS=$(ROMFS)

#prepare the audio file for use, and ensure build exists as well
bannerAudio: $(BANNER_AUDIO) | $(BUILD)
#set it's bitrate and sampling rate correctly
	@$(FFMPEG) -y -loglevel error -i $(BANNER_AUDIO) -ac 2 -ar 44100 -t 3 -c:a pcm_s16le $(TOPDIR)/$(BUILD)/banner.wav
#@$(FFMPEG) -y -loglevel error -i $(TOPDIR)/$(BUILD)/banner.ogg -c:a pcm_s16le $(TOPDIR)/$(BUILD)/banner.wav


#prepare model from blender file to end file type for banner
#bannerModel:

#create the banner
#banner: bannerAudio bannerModel

#Resizes the icon if needed, but it is already 48x48
# $(APP_ICON): $(TOPDIR)/$(ICON)
# 	@mkdir -p $(BUILD)
# 	@$(FFMPEG) -y -i $(TOPDIR)/$(ICON) -vf scale=48:48 -frames:v 1 -update 1 $(APP_ICON)

#create the sdmh file and copy it to the romfs so that launchers and the game itself can read it
#used for homebrew menu
# smdh: $(APP_ICON)
# 	@$(BANNERTOOL) makesdmh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i "$(ICON)" -f $(BANNER_FLAGS) -o $(OUTPUT).sdmh
# 	@cp "$(OUTPUT).sdmh" "$(ROMFS)"

$(BUILD):
	@mkdir -p $@

ifneq ($(GFXBUILD),$(BUILD))
$(GFXBUILD):
	@mkdir -p $@
endif

ifneq ($(MUSICBUILD),$(BUILD))
$(MUSICBUILD):
#TODO: IN HERE, FOR ALL FILES IN MUSIC, CONVERT THEM INTO THE CORRECT AUDIO LIBRARY
	@mkdir -p $@
endif

ifneq ($(DEPSDIR),$(BUILD))
$(DEPSDIR):
	@mkdir -p $@
endif

$(OUTDIR):
	@mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTDIR) $(MUSICBUILD)

#---------------------------------------------------------------------------------
$(GFXBUILD)/%.t3x	$(BUILD)/%.h	:	%.t3s
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@tex3ds -i $< -H $(BUILD)/$*.h -d $(DEPSDIR)/$*.d -o $(GFXBUILD)/$*.t3x

#---------------------------------------------------------------------------------
else

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).3dsx	:	$(OUTPUT).elf $(_3DSXDEPS)

$(OFILES_SOURCES) : $(HFILES)

$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
.PRECIOUS	:	%.t3x %.shbin
#---------------------------------------------------------------------------------
%.t3x.o	%_t3x.h :	%.t3x
#---------------------------------------------------------------------------------
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

#---------------------------------------------------------------------------------
%.shbin.o %_shbin.h : %.shbin
#---------------------------------------------------------------------------------
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
