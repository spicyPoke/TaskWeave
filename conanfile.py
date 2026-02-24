import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.build import check_min_cppstd


class TaskWeaveConan(ConanFile):
    name = "taskweave"
    version = "0.0.1"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
    }

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        # Check minimum C++ standard requirement
        check_min_cppstd(self, "20")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        # Customize the CMakeToolchain to generate presets with arch, os, compiler, and build_type
        tc = CMakeToolchain(self)

        # Generate a unique preset prefix based on settings
        arch = str(self.settings.arch)
        os_name = str(self.settings.os)
        compiler = str(self.settings.compiler)
        build_type = str(self.settings.build_type)

        # Create a preset prefix in the format: conan-<arch>-<os>-<compiler>-<build_type>
        preset_prefix = f"conan-{arch}-{os_name}-{compiler}".lower().replace(".", "")

        # Set the presets prefix to customize the naming convention
        tc.presets_prefix = preset_prefix

        # Enable compile_commands.json generation for IDE integration and tooling
        tc.cache_variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = True

        tc.generate()

    def requirements(self):
        # Add any required packages here
        # For example: self.requires("boost/1.82.0")
        pass

    def build_requirements(self):
        # Add build-specific requirements here
        # For testing
        self.test_requires("gtest/1.14.0")