# Audio Files

Place your audio files here:

## Required Files:

### Background Music (BGM):
- `bgm.mp3` (preferred)
- `bgm.ogg` (alternative)
- `bgm.wav` (alternative)

The game will automatically look for these files in order.

### Sound Effects (Optional):
- `dice_roll.wav` - Sound when rolling dice
- `step.wav` - Sound when player moves
- `ladder.wav` - Sound when using ladder
- `snake.wav` - Sound when hit by snake
- `minigame_start.wav` - Sound when minigame starts
- `minigame_success.wav` - Sound when minigame succeeds
- `minigame_fail.wav` - Sound when minigame fails

## Supported Formats:
- **Music**: MP3, OGG, WAV
- **Sounds**: WAV (recommended for low latency)

## Volume Controls:
- Press `+` or `=` to increase volume
- Press `-` to decrease volume

## Notes:
- If audio files are not found, the game will continue without sound
- SDL2_mixer must be installed for audio to work
  - macOS: `brew install sdl2_mixer`
  - Linux: `sudo apt-get install libsdl2-mixer-dev`
  - Windows: Download SDL2_mixer development libraries

