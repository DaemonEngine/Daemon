/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#include "AudioPrivate.h"

namespace Audio {
    /* When adding an entry point to the audio subsystem,
    make sure the non-null implementation returns early
    when audio subsystem is not initialized.
    See https://github.com/DaemonEngine/Daemon/pull/524 */

    static Cvar::Range<Cvar::Cvar<float>> effectsVolume("audio.volume.effects", "the volume of the effects", Cvar::NONE, 0.8f, 0.0f, 1.0f);

    // We have a big, fixed number of source to avoid rendering too many sounds and slowing down the rest of the engine.
    struct sourceRecord_t {
        AL::Source source;
        std::shared_ptr<Sound> usingSound;
        bool active;
        int priority;
    };

    static sourceRecord_t* sources = nullptr;
    static CONSTEXPR int nSources = 128; //TODO see what's the limit for OpenAL soft

    sourceRecord_t* GetSource(int priority);

    static bool initialized = false;

    void InitSounds() {
        if (initialized) {
            return;
        }

        sources = new sourceRecord_t[nSources];

        for (int i = 0; i < nSources; i++) {
            sources[i].active = false;
        }

        initialized = true;
    }

    void ShutdownSounds() {
        if (not initialized) {
            return;
        }

        delete[] sources;
        sources = nullptr;

        initialized = false;
    }

    void UpdateSounds() {
        if (not initialized) {
            return;
        }

        for (int i = 0; i < nSources; i++) {
            if (sources[i].active) {
                std::shared_ptr<Sound> sound = sources[i].usingSound;

                // Update and Emitter::UpdateSound can call Sound::Stop
                if ( sound->playing ) {
                    sound->Update();
                }

                if ( sound->playing ) {
                    sound->emitter->UpdateSound(*sound);
                }

                if ( !sound->playing ) {
                    sources[i].active = false;
                    sources[i].usingSound = nullptr;
                }
            }
        }
    }

    void StopSounds() {
        if (not initialized) {
            return;
        }

        for (int i = 0; i < nSources; i++) {
            if (sources[i].active) {
                sources[i].usingSound->Stop();
            }
        }
    }

    void AddSound(std::shared_ptr<Emitter> emitter, std::shared_ptr<Sound> sound, int priority) {
        if (not initialized) {
            return;
        }

        sourceRecord_t* source = GetSource(priority);

        if (source) {
            // Make the source forget if it was a "static" or a "streaming" source.
            source->source.ResetBuffer();
            sound->emitter = emitter;
            sound->AcquireSource(source->source);
            source->usingSound = sound;
            source->priority = priority;
            source->active = true;

            sound->FinishSetup();
            sound->Play();
        }
    }

    // Finds a inactive or low-priority source to play a new sound.
    sourceRecord_t* GetSource(int priority) {
        //TODO make a better heuristic? (take into account the distance / the volume /... ?)
        int best = -1;
        int bestPriority = priority;

        // Gets the minimum sound by comparing activity first then priority
        for (int i = 0; i < nSources; i++) {
            sourceRecord_t& source = sources[i];

            if (not source.active) {
                return &source;
            }

            if (source.priority < bestPriority || (best < 0 && source.priority <= priority)) {
                best = i;
                bestPriority = source.priority;
                continue;
            }
        }

        if (best >= 0) {
            sourceRecord_t& source = sources[best];

            source.source.Stop();
            source.source.RemoveAllQueuedBuffers();

            source.usingSound = nullptr;
            return &source;
        } else {
            return nullptr;
        }
    }

    // Implementation of Sound

    Sound::Sound() : positionalGain(1.0f), soundGain(1.0f), currentGain(1.0f),
                     playing(false), volumeModifier(&effectsVolume), source(nullptr) {}

    Sound::~Sound() = default;

    void Sound::Play() {
        source->Play();
        playing = true;
    }

    void Sound::Stop() {
        source->Stop();
        playing = false;
    }

    void Sound::AcquireSource(AL::Source& source) {
        this->source = &source;

        source.SetLooping(false);

        SetupSource(source);
        emitter->SetupSound(*this);
    }

    // Set the gain before the source is started to avoid having a few milliseconds of very loud sound
    void Sound::FinishSetup() {
        currentGain = positionalGain * soundGain * SliderToAmplitude(volumeModifier->Get());
        source->SetGain(currentGain);
    }

    void Sound::Update() {
        // Fade the Gain update to avoid "ticking" sounds when there is a gain discontinuity
        float targetGain = positionalGain * soundGain * SliderToAmplitude(volumeModifier->Get());

        //TODO make it framerate independent and fade out in about 1/8 seconds ?
        if (currentGain > targetGain) {
            currentGain = std::max(currentGain - 0.02f, targetGain);
            //currentGain = std::max(currentGain * 1.05f, targetGain);
        } else if (currentGain < targetGain) {
            currentGain = std::min(currentGain + 0.02f, targetGain);
            //currentGain = std::min(currentGain / 1.05f - 0.01f, targetGain);
        }

        source->SetGain(currentGain);

        InternalUpdate();
    }
    // Implementation of OneShotSound

    OneShotSound::OneShotSound(std::shared_ptr<Sample> sample): sample(sample) {
    }

    OneShotSound::~OneShotSound() = default;

    void OneShotSound::SetupSource(AL::Source& source) {
        source.SetBuffer(sample->GetBuffer());
        soundGain = volumeModifier->Get();
    }

    void OneShotSound::InternalUpdate() {
        if ( source->IsStopped() ) {
            Stop();
            return;
        }
        soundGain = volumeModifier->Get();
    }

    // Implementation of LoopingSound

    LoopingSound::LoopingSound(std::shared_ptr<Sample> loopingSample, std::shared_ptr<Sample> leadingSample)
        : loopingSample(loopingSample),
          leadingSample(leadingSample),
          fadingOut(false) {}

    LoopingSound::~LoopingSound() = default;

    void LoopingSound::FadeOutAndDie() {
        fadingOut = true;
        soundGain = 0.0f;
    }

    void LoopingSound::SetupSource(AL::Source& source) {
        if (leadingSample) {
            source.SetBuffer(leadingSample->GetBuffer());
        } else {
            SetupLoopingSound(source);
        }
        soundGain = volumeModifier->Get();
    }

    void LoopingSound::InternalUpdate() {
        if (fadingOut and currentGain == 0.0f) {
            Stop();
        }

        if (not fadingOut) {
            if (leadingSample) {
                if ( source->IsStopped() ) {
                    SetupLoopingSound( *source );
                    source->Play();
                    leadingSample = nullptr;
                }
            }
            soundGain = volumeModifier->Get();
        }
    }

    void LoopingSound::SetupLoopingSound(AL::Source& source){
        source.SetLooping(true);
        if (loopingSample) {
            source.SetBuffer(loopingSample->GetBuffer());
        }
    }

    // Implementation of StreamingSound

    StreamingSound::StreamingSound() = default;

    StreamingSound::~StreamingSound() = default;

    void StreamingSound::SetupSource(AL::Source&) {
    }

    void StreamingSound::InternalUpdate() {
        while ( source->GetNumProcessedBuffers() > 0 ) {
            source->PopBuffer();
        }

        if ( source->GetNumQueuedBuffers() == 0 ) {
            Stop();
        }
    }

    //TODO somehow try to catch back when data is coming faster than we consume (e.g. capture data)
    void StreamingSound::AppendBuffer(AL::Buffer buffer) {
        if ( !playing ) {
            return;
        }

        source->QueueBuffer(std::move(buffer));

        if ( source->IsStopped() ) {
            source->Play();
        }
    }
}
