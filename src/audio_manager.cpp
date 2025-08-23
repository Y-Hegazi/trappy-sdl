#include "../include/audio_manager.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

AudioManager::AudioManager() : music(nullptr, [](Mix_Music *) {}) {
  // Empty constructor - initialization happens in init()
}

AudioManager::~AudioManager() {
  if (initialized) {
    stopAll();
    sounds.clear();
    music.reset();
    Mix_CloseAudio();
  }
}

void AudioManager::init(int frequency, Uint16 format, int channels,
                        int chunksize) {
  if (initialized) {
    std::cerr << "AudioManager already initialized" << std::endl;
    return;
  }

  // Initialize SDL_mixer
  if (Mix_OpenAudio(frequency, format, channels, chunksize) < 0) {
    throw std::runtime_error("Failed to initialize SDL_mixer: " +
                             std::string(Mix_GetError()));
  }

  initialized = true;
  std::cout << "AudioManager initialized: " << frequency << "Hz, " << channels
            << " channels" << std::endl;
}

bool AudioManager::loadSound(const std::string &id, const char *filePath) {
  if (!initialized) {
    std::cerr << "AudioManager not initialized" << std::endl;
    return false;
  }

  if (!filePath) {
    std::cerr << "Invalid file path for sound: " << id << std::endl;
    return false;
  }

  // Load the chunk
  Mix_Chunk *chunk = Mix_LoadWAV(filePath);
  if (!chunk) {
    std::cerr << "Failed to load sound '" << id << "' from: " << filePath
              << " - " << Mix_GetError() << std::endl;
    return false;
  }

  // Store in map with RAII wrapper
  sounds.emplace(id, makeChunkPtr(chunk));
  std::cout << "Loaded sound: " << id << " from " << filePath << std::endl;
  return true;
}

bool AudioManager::loadMusic(const char *filePath) {
  if (!initialized) {
    std::cerr << "AudioManager not initialized" << std::endl;
    return false;
  }

  if (!filePath) {
    std::cerr << "Invalid file path for music" << std::endl;
    return false;
  }

  // Load the music
  Mix_Music *mus = Mix_LoadMUS(filePath);
  if (!mus) {
    std::cerr << "Failed to load music from: " << filePath << " - "
              << Mix_GetError() << std::endl;
    return false;
  }

  // Store with RAII wrapper
  music = makeMusicPtr(mus);
  std::cout << "Loaded music from " << filePath << std::endl;
  return true;
}

int AudioManager::playSound(const std::string &id, int loops) {
  if (!initialized) {
    std::cerr << "AudioManager not initialized" << std::endl;
    return -1;
  }

  // Find the sound
  auto it = sounds.find(id);
  if (it == sounds.end()) {
    std::cerr << "Sound not found: " << id << std::endl;
    return -1;
  }

  Mix_Chunk *chunk = it->second.get();
  if (!chunk) {
    std::cerr << "Invalid chunk for sound: " << id << std::endl;
    return -1;
  }

  int mixVolume = std::max(0, std::min(soundVolume, MIX_MAX_VOLUME));
  Mix_VolumeChunk(chunk, mixVolume);

  // Play on first available channel
  int channel = Mix_PlayChannel(-1, chunk, loops);
  if (channel == -1) {
    std::cerr << "Failed to play sound '" << id << "': " << Mix_GetError()
              << std::endl;
  }

  return channel;
}

bool AudioManager::playMusic(int loops) {
  if (!initialized) {
    std::cerr << "AudioManager not initialized" << std::endl;
    return false;
  }

  if (!music || !music.get()) {
    std::cerr << "No music loaded" << std::endl;
    return false;
  }

  // Set music volume (0-128)
  int mixVolume = std::max(0, std::min(musicVolume, 128));
  Mix_VolumeMusic(mixVolume);

  // Play the music
  if (Mix_PlayMusic(music.get(), loops) == -1) {
    std::cerr << "Failed to play music: " << Mix_GetError() << std::endl;
    return false;
  }

  return true;
}

void AudioManager::stopAll() {
  if (!initialized)
    return;

  Mix_HaltChannel(-1); // Stop all sound effects
  Mix_HaltMusic();     // Stop music
}

void AudioManager::stopAllSounds() {
  if (!initialized)
    return;

  Mix_HaltChannel(-1); // Stop all sound effects
}

void AudioManager::stopMusic() {
  if (!initialized)
    return;

  Mix_HaltMusic();
}

void AudioManager::setSoundVolume(int volume) {
  if (!initialized) {
    std::cerr << "AudioManager not initialized" << std::endl;
    return;
  }
  soundVolume = std::max(0, std::min(volume, MIX_MAX_VOLUME));
}

void AudioManager::setMusicVolume(int volume) {
  musicVolume = std::max(0, std::min(volume, 128));

  if (initialized) {
    int mixVolume = static_cast<int>(musicVolume * MIX_MAX_VOLUME);
    Mix_VolumeMusic(mixVolume);
  }
}

// Helper functions for RAII wrappers
AudioManager::ChunkPtr AudioManager::makeChunkPtr(Mix_Chunk *chunk) {
  return ChunkPtr(chunk, [](Mix_Chunk *c) {
    if (c) {
      Mix_FreeChunk(c);
    }
  });
}

AudioManager::MusicPtr AudioManager::makeMusicPtr(Mix_Music *music) {
  return MusicPtr(music, [](Mix_Music *m) {
    if (m) {
      Mix_FreeMusic(m);
    }
  });
}
