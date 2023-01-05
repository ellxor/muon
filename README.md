## Muon (pre-alpha software)

Muon is a chess program that aims to be a useful tool for competitive chess players that is widely
available for any OS with minimal dependecies.


**Requirements:**
- C/C++ compiler that supports C23/C++20
- SDL2 (at least version 2.0.17) and SDL2 Image
- Meson/Ninja
  

**Building:**
```
meson setup build/
meson compile -C build/
```

