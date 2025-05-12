release := "release"
xdglocal := "~/.local"

setup buildtype=release prefix=xdglocal *additional:
    meson setup --prefix {{prefix}} --buildtype {{buildtype}} {{additional}} \
        {{ if buildtype == "release" { "-Dmanuals=true" } else { "" } }} \
        {{ if buildtype =~ "debug" { "-Db_sanitize=address,undefined" } else { "" } }} \
        build-{{buildtype}}

build buildtype=release *target:
    meson compile -C build-{{buildtype}} {{target}}

install buildtype=release prefix=xdglocal:
    just setup {{buildtype}} {{prefix}}
    meson install -C build-{{buildtype}}
