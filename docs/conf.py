project = "Silo User Manual"
copyright = "1996-2026, LLNL, LLNL-SM-654357"
author = "LLNL"
release = "4.12.0"

extensions = [
    "sphinx.ext.mathjax",
    "myst_parser",
]

source_suffix = {
    ".md": "markdown",
    ".rst": "restructuredtext",
}

root_doc = "index"

myst_enable_extensions = [
    "deflist",
    "dollarmath",
    "fieldlist",
    "substitution",
    "attrs_block",
    "attrs_inline",
    "colon_fence",
]

myst_heading_anchors = 6

exclude_patterns = [
    "func-template.md",
    "_build",
    "Thumbs.db",
    ".DS_Store",
]

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]
html_theme_options = {
    "navigation_depth": 4,
}

html_css_files = ["custom.css"]
html_js_files = [
    "pagefind-rtd-search-redirect.js",
]
