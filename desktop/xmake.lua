add_rules("mode.release", "mode.debug")

add_requires("libx11", "pulseaudio")
add_requires("pkgconfig::libpulse-simple", {alias="pulse-simple"})

target("jokull")
  set_kind("binary")
  set_languages("c23")
  add_files("main.c", "../common/**.c")
  add_includedirs("../common")
  add_packages("libx11", "pulseaudio", "pulse-simple")
