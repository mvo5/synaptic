cmake_minimum_required(VERSION 3.24)

include(FetchContent)

if(NOT BEMAN_EXEMPLAR_LOCKFILE)
    set(BEMAN_EXEMPLAR_LOCKFILE
        "lockfile.json"
        CACHE FILEPATH
        "Path to the dependency lockfile for the Beman Exemplar."
    )
endif()

set(BemanExemplar_projectDir "${CMAKE_CURRENT_LIST_DIR}/../..")
message(TRACE "BemanExemplar_projectDir=\"${BemanExemplar_projectDir}\"")

message(TRACE "BEMAN_EXEMPLAR_LOCKFILE=\"${BEMAN_EXEMPLAR_LOCKFILE}\"")
file(
    REAL_PATH
    "${BEMAN_EXEMPLAR_LOCKFILE}"
    BemanExemplar_lockfile
    BASE_DIRECTORY "${BemanExemplar_projectDir}"
    EXPAND_TILDE
)
message(DEBUG "Using lockfile: \"${BemanExemplar_lockfile}\"")

# Force CMake to reconfigure the project if the lockfile changes
set_property(
    DIRECTORY "${BemanExemplar_projectDir}"
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS "${BemanExemplar_lockfile}"
)

# For more on the protocol for this function, see:
# https://cmake.org/cmake/help/latest/command/cmake_language.html#provider-commands
function(BemanExemplar_provideDependency method package_name)
    # Read the lockfile
    file(READ "${BemanExemplar_lockfile}" BemanExemplar_rootObj)

    # Get the "dependencies" field and store it in BemanExemplar_dependenciesObj
    string(
        JSON
        BemanExemplar_dependenciesObj
        ERROR_VARIABLE BemanExemplar_error
        GET "${BemanExemplar_rootObj}"
        "dependencies"
    )
    if(BemanExemplar_error)
        message(FATAL_ERROR "${BemanExemplar_lockfile}: ${BemanExemplar_error}")
    endif()

    # Get the length of the libraries array and store it in BemanExemplar_dependenciesObj
    string(
        JSON
        BemanExemplar_numDependencies
        ERROR_VARIABLE BemanExemplar_error
        LENGTH "${BemanExemplar_dependenciesObj}"
    )
    if(BemanExemplar_error)
        message(FATAL_ERROR "${BemanExemplar_lockfile}: ${BemanExemplar_error}")
    endif()

    if(BemanExemplar_numDependencies EQUAL 0)
        return()
    endif()

    # Loop over each dependency object
    math(EXPR BemanExemplar_maxIndex "${BemanExemplar_numDependencies} - 1")
    foreach(BemanExemplar_index RANGE "${BemanExemplar_maxIndex}")
        set(BemanExemplar_errorPrefix
            "${BemanExemplar_lockfile}, dependency ${BemanExemplar_index}"
        )

        # Get the dependency object at BemanExemplar_index
        # and store it in BemanExemplar_depObj
        string(
            JSON
            BemanExemplar_depObj
            ERROR_VARIABLE BemanExemplar_error
            GET "${BemanExemplar_dependenciesObj}"
            "${BemanExemplar_index}"
        )
        if(BemanExemplar_error)
            message(
                FATAL_ERROR
                "${BemanExemplar_errorPrefix}: ${BemanExemplar_error}"
            )
        endif()

        # Get the "name" field and store it in BemanExemplar_name
        string(
            JSON
            BemanExemplar_name
            ERROR_VARIABLE BemanExemplar_error
            GET "${BemanExemplar_depObj}"
            "name"
        )
        if(BemanExemplar_error)
            message(
                FATAL_ERROR
                "${BemanExemplar_errorPrefix}: ${BemanExemplar_error}"
            )
        endif()

        # Get the "package_name" field and store it in BemanExemplar_pkgName
        string(
            JSON
            BemanExemplar_pkgName
            ERROR_VARIABLE BemanExemplar_error
            GET "${BemanExemplar_depObj}"
            "package_name"
        )
        if(BemanExemplar_error)
            message(
                FATAL_ERROR
                "${BemanExemplar_errorPrefix}: ${BemanExemplar_error}"
            )
        endif()

        # Get the "git_repository" field and store it in BemanExemplar_repo
        string(
            JSON
            BemanExemplar_repo
            ERROR_VARIABLE BemanExemplar_error
            GET "${BemanExemplar_depObj}"
            "git_repository"
        )
        if(BemanExemplar_error)
            message(
                FATAL_ERROR
                "${BemanExemplar_errorPrefix}: ${BemanExemplar_error}"
            )
        endif()

        # Get the "git_tag" field and store it in BemanExemplar_tag
        string(
            JSON
            BemanExemplar_tag
            ERROR_VARIABLE BemanExemplar_error
            GET "${BemanExemplar_depObj}"
            "git_tag"
        )
        if(BemanExemplar_error)
            message(
                FATAL_ERROR
                "${BemanExemplar_errorPrefix}: ${BemanExemplar_error}"
            )
        endif()

        if(method STREQUAL "FIND_PACKAGE")
            if(package_name STREQUAL BemanExemplar_pkgName)
                string(
                    APPEND
                    BemanExemplar_debug
                    "Redirecting find_package calls for ${BemanExemplar_pkgName} "
                    "to FetchContent logic.\n"
                )
                string(
                    APPEND
                    BemanExemplar_debug
                    "Fetching ${BemanExemplar_repo} at "
                    "${BemanExemplar_tag} according to ${BemanExemplar_lockfile}."
                )
                message(DEBUG "${BemanExemplar_debug}")
                FetchContent_Declare(
                    "${BemanExemplar_name}"
                    GIT_REPOSITORY "${BemanExemplar_repo}"
                    GIT_TAG "${BemanExemplar_tag}"
                    EXCLUDE_FROM_ALL
                )
                set(INSTALL_GTEST OFF) # Disable GoogleTest installation
                FetchContent_MakeAvailable("${BemanExemplar_name}")

                # Important! <PackageName>_FOUND tells CMake that `find_package` is
                # not needed for this package anymore
                set("${BemanExemplar_pkgName}_FOUND" TRUE PARENT_SCOPE)
            endif()
        endif()
    endforeach()
endfunction()

cmake_language(
    SET_DEPENDENCY_PROVIDER BemanExemplar_provideDependency
    SUPPORTED_METHODS FIND_PACKAGE
)

# Add this dir to the module path so that `find_package(beman-install-library)` works
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}")
