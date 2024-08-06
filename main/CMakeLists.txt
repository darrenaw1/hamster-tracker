idf_component_register(SRCS "ota.c" "main.c" "gpio.c"
                       INCLUDE_DIRS "."
                       EMBED_TXTFILES ${project_dir}/cert/google.cer)
