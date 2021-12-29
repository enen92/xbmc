import os
import re

gh_pages = [
    ['cmake_build_system', '../../cmake/README.md', 'CMake build system', []],
    ['build_fedora', '../README.Fedora.md', 'Fedora build guide', []],
    ['build_android', '../README.Android.md', 'Android build guide', []],
    ['build_freebsd', '../README.FreeBSD.md', 'FreeBSD build guide', []],
    ['build_ios', '../README.iOS.md', 'iOS build guide', []],
    ['build_linux', '../README.Linux.md', 'Linux build guide', []],
    ['build_macos', '../README.macOS.md', 'macOS build guide', []],
    ['build_opensuse', '../README.openSUSE.md', 'openSUSE build guide', []],
    ['build_tvos', '../README.tvOS.md', 'tvOS build guide', []],
    ['build_ubuntu', '../README.Ubuntu.md', 'Debian/Ubuntu build guide', []],
    ['build_windows', '../README.Windows.md', 'Windows build guide', []],
    ['code_guidelines', '../CODE_GUIDELINES.md', 'Code Guidelines', []],
    ['git_fu', '../GIT-FU.md', 'Git tutorial', []],
    ['contributing', '../CONTRIBUTING.md', '3: Contributing', ['git_fu', 'code_guidelines']],
]

def md_to_html_heading(line):
    if line.startswith("##"):
       return line.replace("##","#")
    elif line.startswith("###"):
       return line.replace("###","##")
    elif line.startswith("# "):
        return "<hr>"
    return line

def code(line):
    line = line.replace("```cpp", "```")
    line = line.replace("#pragma", "\#pragma")
    line = line.replace("#ifndef", "\#ifndef")
    line = line.replace("#endif", "\#endif")
    line = line.replace("`/*! */`", "<code>\/\*! \*\/</code>")
    return line

def note_to_doxygen_note(line):
    line = line.replace("**NOTE:**", "@note")
    line = line.replace("**WARNING:**", "@warning")
    line = line.replace("**TIP:**", "@note **TIP:**")
    line = line.replace("**Example:**", "<b>Example:</b>")
    line = line.replace("**Exception:**", "@exception")
    return line

def remove_navigation_links(line):
    if re.findall("\[back to.+?\]", line):
        return ""
    return line

def drop_toc(line):
    if "Table of Contents".lower() in line.lower():
        return ""
    if re.findall("\d+.+?\*\*\[.+?\]\(.+?\)\*\*", line):
        return ""
    return line

def rewrite_images(line):
    line = line.replace("![Kodi Logo](../docs/resources/banner_slim.png)", "![Kodi Logo](../resources/banner_slim.png)")
    line = line.replace("](resources/", "](../resources/")
    return line


def cleanup(content):
    clean_content = []
    for line in content:
        line = md_to_html_heading(line)
        line = note_to_doxygen_note(line)
        line = remove_navigation_links(line)
        line = drop_toc(line)
        line = rewrite_images(line)
        line = code(line)
        clean_content.append(line)
    return clean_content


if __name__ == "__main__":
    for page in gh_pages:
        output_file = os.path.join("pages", "generated", page[0] + ".dox")
        if os.path.exists(output_file):
            os.remove(output_file)

        content = None
        with open(page[1], 'r') as f:
            content = f.readlines()

        if content:
            content = cleanup(content)
            with open(output_file, 'a') as f:
                # preamble
                f.writelines(["/*!\n", f"@page {page[0]} {page[2]} \n"])
                f.writelines(content)
                if page[3]:
                    f.writelines("<div style=display:none;>")
                    for sub_page in page[3]:
                        f.writelines(f"@subpage {sub_page}")
                    f.writelines("</div>")
                # endpage
                f.writelines(["*/"])
    # build doxygen
    os.system("doxygen Doxyfile.doxy")
    # cleanup hacks
    navtree = None
    with open("../html/navtreedata.js", "r") as f:
        navtree = f.readlines()
    if navtree:
        with open("../html/navtreedata.js", "w") as f:
            new_navtree = []
            for line in navtree:
                if "Bug List" not in line and "Todo List" not in line and "Deprecated List" not in line:
                    new_navtree.append(line)
            f.writelines(new_navtree)

