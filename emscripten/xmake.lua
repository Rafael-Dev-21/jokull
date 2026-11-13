add_rules("mode.release", "mode.debug")

target("jokull")
  set_kind("binary")
  set_languages("gnu23")
  add_files("main.c", "../common/*.c")
  add_includedirs("../common")
