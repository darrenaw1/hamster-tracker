idf_component_register(SRCS "server.c" "ota.c" "main.c" "gpio.c" "server.c"
                       INCLUDE_DIRS "."
                       EMBED_TXTFILES ${project_dir}/cert/google.cer)
