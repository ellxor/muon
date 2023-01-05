## Muon (pre-alpha software)

Muon is a chess program that aims to be a useful tool for competitive chess players that is widely
available for any OS with minimal dependencies.


**Requirements:**
- C/C++ compiler that supports C23/C++20 (currently this is only GCC 13)
- SDL2 (at least version 2.0.17) and SDL2 Image
- Meson/Ninja


**Building:**
```
meson setup build/
meson compile -C build/
```

**TODO:**
- [ ] Add proper unit testing for chess move generation
- [ ] Add game tree structure to store game (and variations)
- [ ] Start work on parsing (FEN and PGN) to load positon and games
- [ ] Add text area to display game moves
- [ ] Add support for adding comments to chess positions / moves
- [ ] Add menubar for options (e.g. File -> Load PGN)
- [ ] Add UCI parser to be able to run Stockfish to analyse positions
- [ ] Add text area to display engine analysis
- [ ] Add support for custom chess database format for smaller files (PGNs are quite large)
- [ ] Allow filtering of databases to search for players / positions
- [ ] Add support for building opening trees for opponent preparation

- [ ] Add support for being able to drag pieces rather than clicking
- [ ] Add animations for moving pieces (long-term)
- [ ] Add support for more board color themes (long-term)
- [ ] Add support for move chess piece designs (long-term)
- [ ] Add support for drawing arrows / highlighting squares (long-term)
