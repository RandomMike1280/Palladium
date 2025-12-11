"""
Palladium - Low-level Python UI Library
Build script using setuptools and pybind11
"""

import os
import sys
import subprocess
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class get_pybind_include:
    """Helper class to determine the pybind11 include path."""
    def __str__(self):
        import pybind11
        return pybind11.get_include()


# SDL2 configuration for Windows
def get_sdl2_config():
    """Get SDL2 include and library paths."""
    # Check for SDL2_DIR environment variable first
    sdl2_dir = os.environ.get('SDL2_DIR', '')
    
    if sdl2_dir and os.path.exists(sdl2_dir):
        include_dirs = [os.path.join(sdl2_dir, 'include')]
        library_dirs = [os.path.join(sdl2_dir, 'lib', 'x64')]
    else:
        # Try vcpkg paths
        vcpkg_root = os.environ.get('VCPKG_ROOT', 'C:\\vcpkg')
        triplet = 'x64-windows'
        include_dirs = [os.path.join(vcpkg_root, 'installed', triplet, 'include')]
        library_dirs = [os.path.join(vcpkg_root, 'installed', triplet, 'lib')]
    
    return include_dirs, library_dirs


class BuildExt(build_ext):
    """Custom build extension for handling C++ compilation."""
    
    c_opts = {
        'msvc': ['/EHsc', '/std:c++17', '/O2', '/DNDEBUG'],
        'unix': ['-std=c++17', '-O3', '-fvisibility=hidden'],
    }
    
    l_opts = {
        'msvc': [],
        'unix': [],
    }
    
    def build_extensions(self):
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])
        link_opts = self.l_opts.get(ct, [])
        
        for ext in self.extensions:
            ext.extra_compile_args = opts
            ext.extra_link_args = link_opts
        
        build_ext.build_extensions(self)


# Get SDL2 paths
sdl2_include, sdl2_lib = get_sdl2_config()

ext_modules = [
    Extension(
        'Palladium',
        sources=[
            'src/main.cpp',
            'src/surface.cpp',
            'src/window.cpp',
            'src/animation.cpp',
            'src/effects.cpp',
            'src/layer.cpp',
            'src/material.cpp',
            'src/input.cpp',
            'src/button.cpp',
            'src/font.cpp',
            'src/textfield.cpp',
            # GPU acceleration (Windows only, compiled conditionally)
            'src/d2d_context.cpp',
            'src/gpu_surface.cpp',
            'src/gpu_effects.cpp',
            'src/gpu_window.cpp',
            'src/slider.cpp',
            'src/gpu_text.cpp',
            'src/cpu_text.cpp',
        ],
        include_dirs=[
            get_pybind_include(),
            'src',
        ] + sdl2_include,
        library_dirs=sdl2_lib,
        libraries=['SDL2', 'SDL2_ttf', 'd2d1', 'd3d11', 'dxgi', 'dwrite', 'gdi32'],
        language='c++',
    ),
]

setup(
    name='Palladium',
    version='0.1.0',
    author='User',
    description='Low-level Python UI library with pixel manipulation, animations, and effects',
    long_description='',
    ext_modules=ext_modules,
    install_requires=['pybind11>=2.6.0', 'numpy'],
    cmdclass={'build_ext': BuildExt},
    zip_safe=False,
    python_requires='>=3.8',
)
