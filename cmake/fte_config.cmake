
block(SCOPE_FOR VARIABLES)
	# general rebranding
	set(DISTRIBUTION "FTE") #should be kept short. 8 or less letters is good, with no spaces.
	set(DISTRIBUTIONLONG "Forethought Entertainment") #think of this as your company name. It isn't shown too often, so can be quite long.
	set(FULLENGINENAME "FTE Quake") #nominally user-visible name.
	set(ENGINEWEBSITE "http://fte.triptohell.info") #for shameless self-promotion purposes.
	set(BRANDING_ICON "fte_eukara.ico") #The file to use in windows' resource files - for linux your game should include an icon.[png|ico] file in the game's data.

	# filesystem rebranding
	set(GAME_SHORTNAME "quake") #short alphanumeric description
	set(GAME_FULLNAME "${FULLENGINENAME}") #full name of the game we're playing
	set(GAME_BASEGAMES "${GAME_SHORTNAME}") #comma-separate list of basegame strings to use
	set(GAME_PROTOCOL "FTE-Quake") #so other games won't show up in the server browser
	set(GAME_DEFAULTPORT "27500") #slightly reduces the chance of people connecting to the wrong type of server
	set(GAME_IDENTIFYINGFILES FALSE) #with multiple games, this string-list gives verification that the basedir is actually valid. if null, will just be assumed correct.
	set(GAME_DOWNLOADSURL FALSE) #url for the package manger to update from
	set(GAME_DEFAULTCMDS FALSE) #a string containing the things you want to exec in order to override default.cfg

	set(ENGINE_HAS_ZIP TRUE) #when defined, the engine is effectively a self-extrating zip with the gamedata zipped onto the end (if it in turn contains nested packages then they should probably be STOREd pk3s)

	# Misc Renderer stuff
	set(PSET_CLASSIC TRUE) #support the 'classic' particle system, for that classic quake feel.
	set(PSET_SCRIPT TRUE) #scriptable particles (both fte's and importing effectinfo)
	set(RTLIGHTS TRUE) #
	set(RUNTIMELIGHTING TRUE) #automatic generation of .lit files

	# Extra misc features
	set(CLIENTONLY FALSE) #
	set(MULTITHREAD TRUE) #misc basic multithreading - dsound, downloads, basic stuff that's unlikely to have race conditions.
	set(LOADERTHREAD TRUE) #worker threads for loading misc stuff. falls back on main thread if not supported.
	set(AVAIL_DINPUT TRUE) #
	set(SIDEVIEWS 4) #enable secondary/reverse views.
	set(MAX_SPLITS 4u) #
	set(VERTEXINDEXBYTES 2) #16bit indexes work everywhere but may break some file types, 32bit indexes are optional in gles<=2 and d3d<=9 and take more memory/copying but allow for bigger batches/models. Plugins need to be compiled the same way so this is no longer set per-renderer.
	set(TEXTEDITOR TRUE) #my funky text editor! its awesome!
	set(PLUGINS TRUE) #support for external plugins (like huds or fancy menus or whatever)
	set(USE_SQLITE TRUE) #sql-database-as-file support
	set(IPLOG TRUE) #track player's ip addresses (any decent server will hide ip addresses, so this probably isn't that useful, but nq players expect it)

	# Filesystem formats
	set(PACKAGE_PK3 TRUE) #aka zips. we support utf8,zip64,spans,weakcrypto,(deflate),(bzip2),symlinks. we do not support strongcrypto nor any of the other compression schemes.
	set(PACKAGE_Q1PAK TRUE) #also q2
	set(PACKAGE_DOOMWAD FALSE) #doom wad support (generates various file names, and adds support for doom's audio, sprites, etc)
	set(AVAIL_XZDEC TRUE) #.xz decompression
	set(AVAIL_GZDEC TRUE) #.gz decompression
	set(AVAIL_ZLIB TRUE) #whether pk3s can be compressed or not.
	set(AVAIL_BZLIB FALSE) #whether pk3s can use bz2 compression
	set(PACKAGE_DZIP TRUE) #.dzip support for smaller demos (which are actually more like pak files and can store ANY type of file)

	# Map formats
	set(Q1BSPS TRUE) #Quake1
	set(Q2BSPS TRUE) #Quake2
	set(Q3BSPS TRUE) #Quake3, as well as a load of other games too...
	set(RFBSPS TRUE) #qfusion's bsp format / jk2o etc.
	set(TERRAIN TRUE) #FTE's terrain, as well as .map support
	set(DOOMWADS FALSE) #map support, filesystem support is separate.
	set(MAP_PROC FALSE) #doom3...

	# Model formats
	set(SPRMODELS TRUE) #Quake's sprites
	set(SP2MODELS TRUE) #Quake2's models
	set(DSPMODELS TRUE) #Doom sprites!
	set(MD1MODELS TRUE) #Quake's models.
	set(MD2MODELS TRUE) #Quake2's models
	set(MD3MODELS TRUE) #Quake3's models, also often used for q1 etc too.
	set(MD5MODELS TRUE) #Doom3 models.
	set(ZYMOTICMODELS TRUE) #nexuiz uses these, for some reason.
	set(DPMMODELS TRUE) #these keep popping up, despite being a weak format.
	set(PSKMODELS TRUE) #unreal's interchange format. Undesirable in terms of load times.
	set(HALFLIFEMODELS TRUE) #horrible format that doesn't interact well with the rest of FTE's code. Unusable tools (due to license reasons).
	set(INTERQUAKEMODELS TRUE) #Preferred model format, at least from an idealism perspective.
	set(MODELFMT_MDX TRUE) #kingpin's format (for hitboxes+geomsets).
	set(MODELFMT_OBJ TRUE) #lame mesh-only format that needs far too much processing and even lacks a proper magic identifier too
	set(MODELFMT_GLTF TRUE) #khronos 'transmission format'. .gltf or .glb extension. PBR. Version 2 only, for now.
	set(RAGDOLL TRUE) #ragdoll support. requires RBE support (via a plugin...).

	# Image formats
	set(IMAGEFMT_KTX TRUE) #Khronos TeXture. common on gles3 devices for etc2 compression
	set(IMAGEFMT_PKM TRUE) #file format generally written by etcpack or android's etc1tool. doesn't support mips.
	set(IMAGEFMT_ASTC TRUE) #lame simple header around a single astc image. not needed for astc in ktx files etc. its better to use ktx files.
	set(IMAGEFMT_PBM TRUE) #pbm/ppm/pgm/pfm family formats.
	set(IMAGEFMT_PSD TRUE) #baselayer only.
	set(IMAGEFMT_XCF TRUE) #flattens, most of the time
	set(IMAGEFMT_HDR TRUE) #an RGBE format.
	set(IMAGEFMT_DDS TRUE) #.dds files embed mipmaps and texture compression. faster to load.
	set(IMAGEFMT_TGA TRUE) #somewhat mandatory
	set(IMAGEFMT_LMP TRUE) #mandatory for quake
	set(IMAGEFMT_PNG TRUE) #common in quakeworld, useful for screenshots.
	set(IMAGEFMT_JPG TRUE) #common in quake3, useful for screenshots.
	set(IMAGEFMT_GIF FALSE) #for the luls. loads as a texture2DArray
	set(IMAGEFMT_BLP FALSE) #legacy crap
	set(IMAGEFMT_BMP TRUE) #windows bmp. yuck. also includes .ico for the luls
	set(IMAGEFMT_PCX TRUE) #paletted junk. required for qw player skins, q2 and a few old skyboxes.
	set(IMAGEFMT_EXR TRUE) #openexr, via Industrial Light & Magic's rgba api, giving half-float data.
	set(IMAGEFMT_PVR TRUE) #powervr texture, used by various dreamcast games including HL and Q3
	set(AVAIL_PNGLIB TRUE) #.png image format support (read+screenshots)
	set(AVAIL_JPEGLIB TRUE) #.jpeg image format support (read+screenshots)
	set(AVAIL_STBI FALSE) #make use of Sean T. Barrett's lightweight public domain stb_image[_write] single-file-library, to avoid libpng/libjpeg dependancies.
	set(PACKAGE_TEXWAD TRUE) #quake's image wad support
	set(AVAIL_FREETYPE TRUE) #for truetype font rendering
	set(DECOMPRESS_ETC2 TRUE) #decompress etc2(core in gles3/gl4.3) if the graphics driver doesn't support it (eg d3d or crappy gpus with vulkan).
	set(DECOMPRESS_S3TC TRUE) #allows bc1-3 to work even when drivers don't support it. This is probably only an issue on mobile chips. WARNING: not entirely sure if all patents expired yet...
	set(DECOMPRESS_RGTC TRUE) #bc4+bc5
	set(DECOMPRESS_BPTC TRUE) #bc6+bc7
	set(DECOMPRESS_ASTC TRUE) #ASTC, for drivers that don't support it properly.

	# Game/Gamecode Support
	set(CSQC_DAT TRUE) #
	set(MENU_DAT TRUE) #
	set(MENU_NATIVECODE FALSE) #Use an external dll for menus.
	set(VM_Q1 TRUE) #q1qvm implementation, to support ktx.
	set(VM_LUA FALSE) #optionally supports lua instead of ssqc.
	set(Q2SERVER TRUE) #q2 server+gamecode.
	set(Q2CLIENT TRUE) #q2 client. file formats enabled separately.
	set(Q3CLIENT TRUE) #q3 client stuff.
	set(Q3SERVER TRUE) #q3 server stuff.
	set(AVAIL_BOTLIB TRUE) #q3 botlib
	set(BOTLIB_STATIC FALSE) #should normally be set only in the makefile, and only if AVAIL_BOTLIB is defined above.
	set(HEXEN2 TRUE) #runs hexen2 gamecode, supports hexen2 file formats.
	set(HUFFNETWORK TRUE) #crappy network compression. probably needs reseeding.
	set(NETPREPARSE TRUE) #allows for running both nq+qw on the same server (if not, protocol used must match gamecode).
	set(SUBSERVERS TRUE) #Allows the server to fork itself, each acting as an MMO-style server instance of a single 'realm'.
	set(HLCLIENT FALSE) #we can run HL gamecode (not protocol compatible, set to 6 or 7)
	set(HLSERVER FALSE) #we can run HL gamecode (not protocol compatible, set to 138 or 140)
	set(SAVEDGAMES TRUE) #Can save the game.
	set(MVD_RECORDING TRUE) #server can record MVDs.
	set(ENGINE_ROUTING TRUE) #Engine-provided routing logic (possibly threaded)
	set(USE_INTERNAL_BULLET FALSE) #Statically link against bullet physics plugin (instead of using an external plugin)
	set(USE_INTERNAL_ODE FALSE) #Statically link against ode physics plugin (instead of using an external plugin)

	# Networking options
	set(NQPROT TRUE) #act as an nq client/server, with nq gamecode.
	set(HAVE_PACKET TRUE) #we can send unreliable messages!
	set(HAVE_TCP TRUE) #we can create/accept TCP connections.
	set(HAVE_GNUTLS TRUE) #on linux
	set(HAVE_WINSSPI TRUE) #on windows
	set(FTPSERVER TRUE) #sv_ftp cvar.
	set(WEBCLIENT TRUE) #uri_get+any internal downloads etc
	set(HAVE_HTTPSV TRUE) #net_enable_http/websocket
	set(TCPCONNECT TRUE) #support for playing over tcp sockets, instead of just udp. compatible with qizmo.
	set(IRCCONNECT FALSE) #lame support for routing game packets via irc server. not a good idea.
	set(SUPPORT_ICE TRUE) #Internet Connectivity Establishment, for use by plugins to establish voice or game connections.
	set(CL_MASTER TRUE) #Clientside Server Browser functionality.
	set(PACKAGEMANAGER TRUE) #Allows the user to enable/disable/download(with WEBCLIENT) packages and plugins. Also handles map packages.

	# Audio Drivers
	set(AVAIL_OPENAL TRUE) #
	set(AVAIL_WASAPI TRUE) #windows advanced sound api
	set(AVAIL_DSOUND TRUE) #
	set(HAVE_MIXER TRUE) #support non-openal audio drivers

	# Audio Formats
	set(AVAIL_OGGVORBIS TRUE) #.ogg support
	set(AVAIL_MP3_ACM TRUE) #.mp3 support (windows only).

	# Other Audio Options
	set(VOICECHAT TRUE) #
	set(HAVE_SPEEX TRUE) #Support the speex codec.
	set(HAVE_OPUS TRUE) #Support the opus codec.
	set(HAVE_MEDIA_DECODER TRUE) #can play cin/roq, more with plugins
	set(HAVE_MEDIA_ENCODER TRUE) #capture/capturedemo work.
	set(HAVE_CDPLAYER TRUE) #includes cd playback. actual cds. named/numbered tracks are supported regardless (though you need to use the 'music' command to play them without this).
	set(HAVE_JUKEBOX TRUE) #includes built-in jukebox crap
	set(HAVE_SPEECHTOTEXT TRUE) #windows speech-to-text thing

	# Features required by vanilla quake/quakeworld...
	set(QUAKETC FALSE) #
	set(QUAKESTATS TRUE) #defines STAT_HEALTH etc. if omitted, you'll need to provide that functionality yourself.
	set(QUAKEHUD TRUE) #support for drawing the vanilla hud. disable this if you're always going to be using csqc (or equivelent)
	set(QWSKINS TRUE) #disabling this means no qw .pcx skins nor enemy/team skin/colour forcing
	set(NOBUILTINMENUS FALSE) #
	set(NOLEGACY FALSE) #just spike trying to kill off crappy crap...
	set(USEAREAGRID TRUE) #world collision optimisation. REQUIRED for performance with xonotic. hopefully it helps a few other mods too.
	set(NOQCDESCRIPTIONS FALSE) #if 2, disables writing fteextensions.qc completely. 1 just omits the text. (ignored in debug builds.)

	# Outdated stuff
	set(SVRANKING TRUE) #legacy server-side ranking system.
	set(QTERM FALSE) #qterm... adds a console command that allows running programs from within quake - bit like xterm.
	set(SVCHAT FALSE) #ancient lame builtin to support NPC-style chat...
	set(SV_MASTER FALSE) #Support running the server as a master server. Should probably not be used.
	set(QUAKESPYAPI FALSE) #define this if you want the engine to be usable via gamespy/quakespy, which has been dead for a long time now. forces the client to use a single port for all outgoing connections, which hurts reconnects.

	# configure the options header
	configure_file(${FTE_ROOT_DIR}/cmake/config.h.in ${FTE_ENGINE_COMMON_DIR}/config.h @ONLY)
endblock()
