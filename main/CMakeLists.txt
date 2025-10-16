idf_component_register(SRCS "https_server.c" "wifi_app.c" "i2c_interface.c" "main.c" 
                        "max30102.c"
                    INCLUDE_DIRS "." "../include"
                    EMBED_FILES webpage/app.css webpage/app.js webpage/favicon.ico webpage/index.html webpage/jquery-3.3.1.min.js)
