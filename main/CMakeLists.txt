set(EXTRA_COMPONENT_DIRS components)

idf_component_register(SRCS "frames.c" "oled_anim.c" "main.c"
                    INCLUDE_DIRS ".")