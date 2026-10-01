# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information
import os
# Sửa tên project bỏ chữ Documentation thừa
project = 'Robot Hub'
copyright = '2026, ThanggTrann12'
author = 'ThanggTrann12'

# Định nghĩa chính xác tiêu đề hiển thị trên Sidebar
html_title = 'Robot Hub'
html_theme = 'furo'
html_sidebars = {
    "**": [
        "sidebar/brand.html",
        "language-switcher.html",  # Nhúng nút chọn ngôn ngữ vào đây
        "sidebar/search.html",
        "sidebar/scroll-start.html",
        "sidebar/navigation.html",
        "sidebar/ethical-ads.html",
        "sidebar/scroll-end.html",
    ]
}
# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.ifconfig',
    'breathe',
]

templates_path = ['_templates']
exclude_patterns = []

language = 'en'
locale_dirs = ['../locales/']   # Thư mục chứa bản dịch
gettext_compact = False     # Tách file dịch theo từng trang .rst

html_static_path = ['_static']
html_css_files = [
    'custom.css',
]
# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_static_path = ['_static', 'images']

breathe_projects = {
    "RobotHub": os.path.abspath("../_build/doxygen/xml")
}
breathe_default_project = "RobotHub"

DOXYGEN_INDEX = os.path.abspath("../_build/doxygen/xml/index.xml")
has_doxygen_xml = os.path.exists(DOXYGEN_INDEX)

API_AUTOGEN_PATH = os.path.join(os.path.dirname(__file__), "_api_autogen.rst")
if has_doxygen_xml:
     api_autogen = """API Index
---------

.. doxygenindex::
    :project: RobotHub

Core Interfaces
---------------

.. doxygenclass:: IMotor
    :project: RobotHub
    :members:

.. doxygenclass:: ComManager
    :project: RobotHub
    :members:

Motor Drivers
-------------

.. doxygenclass:: MotorTA6586
    :project: RobotHub
    :members:

.. doxygenclass:: PCA9685Motor
    :project: RobotHub
    :members:

Input Components
----------------

.. doxygenclass:: Swc_Button
    :project: RobotHub
    :members:

.. doxygenclass:: Swc_Joystick
    :project: RobotHub
    :members:

Shared Data Types
-----------------

.. doxygenstruct:: ControlPacket
    :project: RobotHub
    :members:

.. doxygenstruct:: PairPacket
    :project: RobotHub
    :members:
"""
else:
     api_autogen = """Doxygen XML was not found, so API content cannot be rendered yet.

To generate API docs from C++ comments:

1. Install Doxygen and add it to PATH.
2. Run ``doxygen Doxyfile`` at repository root.
3. Rebuild Sphinx docs.
"""

with open(API_AUTOGEN_PATH, "w", encoding="utf-8") as api_file:
     api_file.write(api_autogen)