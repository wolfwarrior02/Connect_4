# Connect 4 Game - Enhanced Terminal Graphics

## Project Overview

This is a Connect 4 game implementation using Linux system calls (IPC - shared memory and semaphores) with an enhanced terminal-based graphical interface.

### How the Project Works

The game consists of three main components:

1. **F4Server.c** - Game server that:
   - Manages the game state in shared memory
   - Validates moves and checks for wins
   - Coordinates turn-based gameplay between clients
   - Uses semaphores for process synchronization

2. **F4Client.c** - Player client that:
   - Connects to the server via shared memory
   - Displays the enhanced game board
   - Handles player input and moves
   - Shows game status and results

3. **F4Bot.c** - AI bot player that:
   - Makes random moves automatically
   - Can play against human players

### Enhanced Graphics Features

The terminal interface has been significantly improved with:

#### Visual Enhancements:
- **Unicode Box-Drawing Characters**: Professional grid layout with ┌─┬─┐ style borders
- **Color-Coded Game Pieces**: 
  - Red ● for X player
  - Blue ● for O player
- **Decorative Game Title**: Beautiful header with Unicode borders
- **Column Numbering**: Clear yellow-colored numbers in header and footer
- **Enhanced Spacing**: Proper alignment and visual hierarchy

#### User Experience Improvements:
- **Clear Turn Indicators**: Colored prompts showing whose turn it is
- **Better Input Prompts**: "Enter column number (1-7)" instead of cryptic messages
- **Enhanced Error Messages**: Color-coded warnings with icons (⚠️)
- **Improved Game Results**: Celebration messages with emojis (🎉, 😞, 🤝)
- **Visual Status Updates**: Better waiting messages with icons (✓, ⏳)

### How to Play

1. **Start the server:**
   ```bash
   ./server rows columns token1 token2
   # Example: ./server 6 7 X O
   ```

2. **Connect first player:**
   ```bash
   ./client PlayerName
   ```

3. **Connect second player or bot:**
   ```bash
   ./client Player2    # For human vs human
   # OR
   ./bot              # For human vs bot
   ```

### Technical Implementation

The enhanced graphics are implemented through:

- **ANSI Color Codes**: Terminal colors for pieces and messages
- **Unicode Characters**: Box-drawing characters for professional appearance
- **Enhanced Display Functions**: Improved `stampaCampo()` with better layout
- **Color-Coded Messages**: Different colors for different types of information

### Before and After Comparison

**Original Display:**
```
[ ][ ][ ][ ][ ][ ][ ]
[ ][ ][ ][ ][ ][ ][ ]
[ ][ ][ ][ ][ ][ ][ ]
[ ][X][ ][ ][ ][ ][ ]
[O][X][O][ ][ ][ ][ ]
[O][X][O][X][ ][ ][ ]
```

**Enhanced Display:**
```
╔═══════════════════════════════════╗
║          CONNECT 4 GAME           ║
╚═══════════════════════════════════╝

    1  2  3  4  5  6  7 
  ┌───┬───┬───┬───┬───┬───┬───┐
  │   │   │   │   │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
  │   │   │   │   │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
  │   │ ● │   │   │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
  │ ● │ ● │ ● │   │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
  │ ● │ ● │ ● │ ● │   │   │   │
  └───┴───┴───┴───┴───┴───┴───┘

    1  2  3  4  5  6  7 
```

### Compilation

```bash
gcc -o server F4Server.c
gcc -o client F4Client.c  
gcc -o bot F4Bot.c
```

The enhanced graphics maintain full compatibility with the original game logic while providing a much more engaging and user-friendly experience.