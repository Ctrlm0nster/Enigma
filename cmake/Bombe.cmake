target_sources(EnigmaCore PRIVATE
  src/bombe/Bombe.cpp
  src/bombe/DecipherInput.cpp
)

add_executable(EnigmaBombe src/bombe/main.cpp)
target_link_libraries(EnigmaBombe PRIVATE EnigmaCore)
enigma_set_warnings(EnigmaBombe)
enigma_enable_sanitizers(EnigmaBombe)

find_package(Threads REQUIRED)
if(TARGET Enigma)
  target_sources(Enigma PRIVATE src/bombe/DecipherPanel.cpp)
  target_link_libraries(Enigma PRIVATE Threads::Threads)
endif()

if(TARGET EnigmaTests)
  target_sources(EnigmaTests PRIVATE test/bombe_tests.cpp)
  target_link_libraries(EnigmaTests PRIVATE Threads::Threads)
  if(ENIGMA_WORKBENCH_AVAILABLE)
    target_sources(EnigmaTests PRIVATE
      src/bombe/DecipherPanel.cpp
      test/bombe_panel_tests.cpp
    )
  endif()
endif()
