#Finds Python and creates a minimal python environment for embedding.
#Only implemented for MinGW, needs testing for linux, we don't support visual studio anyway

function(bundle_python_environment TARGET_NAME)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

    # Get the source of the standard library (e.g., /mingw64/lib/python3.1)
    execute_process(
            COMMAND "${Python3_EXECUTABLE}" -c "import sysconfig; print(sysconfig.get_path('stdlib'), end='')"
            OUTPUT_VARIABLE PY_SRC_STDLIB
    )

    # Extract version name (python3.12)
    get_filename_component(PY_VER_NAME "${PY_SRC_STDLIB}" NAME)

    set(PY_TEMP_DIR "${CMAKE_BINARY_DIR}/python_temp_build") #used for compressing, then deleted
    set(PY_DIST_DIR "${CMAKE_BINARY_DIR}/Python")
    set(PY_DIST_LIB "${PY_DIST_DIR}/lib/${PY_VER_NAME}")

    # -------------------------------------------------------------------------
    # 2. DEFINE BLOAT TO REMOVE
    # -------------------------------------------------------------------------
    set(EXCLUSIONS
            "**/test/**"
            "**/tests/**"
            "**/__pycache__/**"
            "**/idlelib/**"
            "**/distutils/**"
            "**/tkinter/**"
            "**/turtledemo/**"
            "**/ensurepip/**"
            "**/venv/**"
            "**/pydoc_data/**"
            "**/unittest/**"
            "**/email/**"
            "**/html/**"
            "**/http/**"
            "**/xml/**"
            "*.pyc"
            "*.pyo"
    )


    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMENT "Bundling Minimal Python Environment..."

            # A. Cleanup previous build
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${PY_TEMP_DIR}"
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${PY_DIST_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PY_TEMP_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PY_DIST_LIB}"

            # B. Copy FULL Standard Lib to Temp
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${PY_SRC_STDLIB}" "${PY_TEMP_DIR}"
    )

    #delete excluded files
    foreach(PATTERN ${EXCLUSIONS})
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf "${PY_TEMP_DIR}/${PATTERN}"
        )
    endforeach()

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMENT "Compressing Python..."

            # 1. Move Binary Extensions (lib-dynload) OUT of the temp folder
            # (Python cannot load DLLs from inside a zip)
            COMMAND ${CMAKE_COMMAND} -E rename
            "${PY_TEMP_DIR}/lib-dynload"
            "${PY_DIST_LIB}/lib-dynload"

            # 2. Zip the remaining .py files
            COMMAND ${CMAKE_COMMAND} -E chdir "${PY_TEMP_DIR}"
            ${CMAKE_COMMAND} -E tar "cf" "${PY_DIST_LIB}/site-packages.zip" --format=zip .

            # 3. Cleanup Temp
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${PY_TEMP_DIR}"
    )

    # 4. HANDLE MAIN DLL (Windows/MinGW Specific)
    if(WIN32)
        # MSYS2 usually puts the DLL in /bin, not /lib. We attempt to find it.
        get_filename_component(PY_LIB_PARENT "${PY_SRC_STDLIB}" DIRECTORY)
        get_filename_component(PY_ROOT "${PY_LIB_PARENT}" DIRECTORY)
        set(MSYS_DLL "${PY_ROOT}/bin/libpython${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}.dll")

        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMENT "Copying Python DLL..."
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MSYS_DLL}"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/"
        )
    endif()

endfunction()