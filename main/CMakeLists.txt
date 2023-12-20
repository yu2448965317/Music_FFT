file(GLOB_RECURSE WS2812 WS2812/*.c)
file(GLOB_RECURSE WIFI WIFI/*.c)
file(GLOB_RECURSE FILE_SERVER FILE_SERVER/*.c)
file(GLOB_RECURSE UI UI/*.c)
idf_component_register(SRCS 
"lvgl_gui.c"
"main.c"
"freetype/lv_freetype.c" 
"tp_interrupt.c"
${WS2812}
${WIFI}
${FILE_SERVER}
${UI}
INCLUDE_DIRS "." 
EMBED_FILES "favicon.ico" "upload_script.html"
 )
