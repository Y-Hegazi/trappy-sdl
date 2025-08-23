#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "config.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <memory>
#include <string>
#include <unordered_map>

/**
 * AudioManager - Centralized audio resource management for SDL_mixer
 *
 * Features:
 * - RAII wrappers for Mix_Chunk and Mix_Music with automatic cleanup
 * - Sound effect loading and playing by string ID
 * - Channel management and volume control
 * - Exception-safe initialization and cleanup
 *
 * Usage:
 * AudioManager audio;
 * audio.init();
 * audio.loadSound("jump", "sounds/jump.wav");
 * audio.playSound("jump", 1.0f);
 */
class AudioManager {
public:
  /**
   * Initialize SDL_mixer audio subsystem
   * @param frequency Sample rate (default: 44100)
   * @param format Audio format (default: MIX_DEFAULT_FORMAT)
   * @param channels Audio channels (default: 2 for stereo)
   * @param chunksize Buffer size (default: 2048)
   * @throws std::runtime_error if initialization fails
   */
  void init(int frequency = 44100, Uint16 format = MIX_DEFAULT_FORMAT,
            int channels = 2, int chunksize = 512);

  /**
   * Load a sound effect from file and register it with an ID
   * @param id String identifier for the sound
   * @param filePath Path to audio file (WAV, OGG, etc.)
   * @return true if loaded successfully, false otherwise
   */
  bool loadSound(const std::string &id, const char *filePath);

  /**
   * Load background music from file
   * @param filePath Path to music file (MP3, OGG, WAV, etc.)
   * @return true if loaded successfully, false otherwise
   */
  bool loadMusic(const char *filePath);

  /**
   * Play a sound effect by ID
   * @param id Sound identifier previously loaded
   * @param volume Volume multiplier (0.0 to 1.0)
   * @param loops Number of extra loops (-1 for infinite, 0 for play once)
   * @return Channel number used, or -1 on error
   */
  int playSound(const std::string &id, int loops = 0);

  /**
   * Play background music
   * @param loops Number of loops (-1 for infinite, 0 for play once)
   * @return true if music started successfully
   */
  bool playMusic(int loops = -1);

  /**
   * Stop all playing sounds and music
   */
  void stopAll();

  /**
   * Stop all sound effects (but not music)
   */
  void stopAllSounds();

  /**
   * Stop background music
   */
  void stopMusic();

  /**
   * Set master volume for sound effects
   * @param volume Volume (0.0 to 1.0)
   */
  void setSoundVolume(int volume);

  /**
   * Set master volume for music
   * @param volume Volume (0.0 to 1.0)
   */
  void setMusicVolume(int volume);

  /**
   * Check if audio system is initialized
   */
  bool isInitialized() const { return initialized; }

  // Rule of 5: proper resource management
  AudioManager();
  ~AudioManager();
  AudioManager(const AudioManager &) = delete;
  AudioManager &operator=(const AudioManager &) = delete;
  AudioManager(AudioManager &&) = delete;
  AudioManager &operator=(AudioManager &&) = delete;

private:
  bool initialized = false;

  // Smart pointer type for Mix_Chunk with custom deleter
  using ChunkPtr = std::unique_ptr<Mix_Chunk, void (*)(Mix_Chunk *)>;

  // Smart pointer type for Mix_Music with custom deleter
  using MusicPtr = std::unique_ptr<Mix_Music, void (*)(Mix_Music *)>;

  // Sound effects storage
  std::unordered_map<std::string, ChunkPtr> sounds;

  // Background music
  MusicPtr music;

  // Volume settings
  int soundVolume = SOUND_EFFECT_VOLUME;
  int musicVolume = MUSIC_VOLUME;

  /**
   * Helper to create ChunkPtr with proper deleter
   */
  static ChunkPtr makeChunkPtr(Mix_Chunk *chunk);

  /**
   * Helper to create MusicPtr with proper deleter
   */
  static MusicPtr makeMusicPtr(Mix_Music *music);
};

// Player sound effect IDs - use these constants for consistency
namespace PlayerSounds {
constexpr const char *DEATH = "player_death";
constexpr const char *DASH = "player_dash";
constexpr const char *COLLECT_COIN = "player_collect_coin";
constexpr const char *HIT_BY_ARROW = "player_hit_by_arrow";
constexpr const char *DEAD_BY_TRAP = "player_dead_by_trap";
constexpr const char *JUMP = "player_jump";
constexpr const char *WIN = "player_win";
} // namespace PlayerSounds

#endif // AUDIO_MANAGER_H
