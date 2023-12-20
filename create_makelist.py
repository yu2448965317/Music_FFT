import os
c_file = "idf_component_register(SRCS "
cur_dir = os.getcwd()+"\main"
makelist_file = cur_dir + "\CMakeLists.txt"
list_filename = os.listdir(cur_dir)
for i in list_filename:
        if ".c" in i :
            c_file = c_file + '"'+i+'"'+'\n'
c_file= c_file+'INCLUDE_DIRS ".")'
with open(makelist_file, 'w') as f:
         f.write(c_file)
# idf_component_register(SRCS "ui_comp.c" "ui.c" "ui_comp_hook.c" "main.c" 
#                             "esp_lcd_panel_hx8369.c" 
#                             "lvgl_gui.c"
#                             "call_back.c"
#                             "diy_widget.c"
#                             "systick.c"
#                             "esp32_mcpwm.c"
#                             "BLDCMotor.c"
#                             "AS5600.c"
#                             "foc_utils.c"
#                             "FOCMotor.c"
#                             "lowpass_filter.c"
#                             "pid.c"
#                             "ui.c"
#                             "ui_helpers.c"
#                             "ui_font_Number.c"
#                             "ui_events.c"
#                             "ui_comp.c"
#                             "ui_comp_hook.c"  
#                             "ui_img_btn_print_down_png.c"      
#                             "ui_font_Small_Font.c"                 
# INCLUDE_DIRS ".")