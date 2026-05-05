import os, sys

project = u'Silo User Manual'
copyright = u'1996-2023, LLNL, LLNL-SM-654357'
author = u'LLNL'
release = u'4.11.1'

extensions = [
    'sphinx.ext.mathjax',
    'copy_pagefind_anchors'
]

#
# Add directory containin python extensions for Sphinx
#
sys.path.insert(0, os.path.abspath("_ext"))

if os.environ.get('READTHEDOCS'):
    from subprocess import call
    call(['pip', 'install', 'myst_parser'])
extensions.append('myst_parser')

# Attempt add_css_file only if the version of Sphinx actually supports it
def setup(app):
    if hasattr(app, 'add_css_file'):
        app.add_css_file('custom.css')
    elif hasattr(app, 'add_stylesheet'):
        app.add_stylesheet('custom.css')

# Files to exclude
exclude_patterns = [ "func-template.md" ]

# Using default suffix would have been fine, but root_doc
# variable winds up assuming `.rst` without this.
source_suffix = {
    '.md': 'markdown'
}

# Top of MyST/Sphinx doc tree WITHOUT the extension
#root_doc = u'index-sphinx'

# MYST specific extensions
myst_enable_extensions = [
    "deflist",
    "dollarmath",
    "fieldlist",
    "substitution",
    "attrs_block",
    "attrs_inline",
    "colon_fence"
]
myst_heading_anchors = 3

myst_substitutions = {
  "EndFunc": "<hr class=\"docutils\" />"+"<br>"*40
}

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
try:
    html_theme
except:
    html_theme = "sphinx_rtd_theme"

html_static_path = ['_static']
html_theme_options = {
    # Toc options
    'navigation_depth': 4
}

html_js_files = ["redirect-rtd-search-to-pagefind.js"]
