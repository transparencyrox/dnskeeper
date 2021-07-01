install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/etc/
    DESTINATION www
    FILES_MATCHING 
        PATTERN "*.css" 
        PATTERN "*.tpl"
        PATTERN "*.html"
)