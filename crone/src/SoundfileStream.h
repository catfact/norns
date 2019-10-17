//
// Created by emb on 10/13/19.
//

#ifndef CRONE_SOUNDFILESTREAM_H
#define CRONE_SOUNDFILESTREAM_H

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <string>

#include <jack/types.h>
#include <jack/ringbuffer.h>
#include <sndfile.h>

#include "Window.h"

namespace crone {

    template <int numChannels, int bufFrames>
    class SoundfileStream {
//        friend  class SoundfileReader;
//        friend  class SoundfileWriter;
    protected:
        typedef jack_default_audio_sample_t Sample;
        static constexpr size_t sampleSize = sizeof(Sample);
        static constexpr size_t frameSize = sampleSize * numChannels;
        static constexpr size_t ringBufFrames = 16384;
        static constexpr size_t ringBufBytes = sampleSize * numChannels * ringBufFrames;

    protected:
        SNDFILE *file;
        std::unique_ptr<std::thread> th;
        std::mutex mut;
        std::condition_variable cv;
        std::unique_ptr<jack_ringbuffer_t> ringBuf;
        volatile int status;

//        typedef enum {
//            Starting, Playing, Stopping, Stopped
//        } EnvState;
//
//        std::atomic<EnvState> envState;
//        std::atomic<int> envIdx;

    public:
        std::atomic<bool> isRunning;
        std::atomic<bool> shouldStop;

    public:
        SoundfileStream():
                file(nullptr),
                status(0),
                isRunning(false),
                shouldStop(false)
        {
            ringBuf = std::unique_ptr<jack_ringbuffer_t>(jack_ringbuffer_create(ringBufBytes));
            envIdx = 0;
            envState = Stopped;
        }

        // from any thread
        virtual void start() {
            if (isRunning) {
                return;
            } else {
                envIdx = 0;
                envState = Starting;
                this->th = std::make_unique<std::thread>(
                        [this]() {
                            this->diskLoop();
                        });
                this->th->detach();
            }
        }

        // from any thread
        void stop() {
            envState = Stopping;
        }

    protected:

        virtual void diskLoop() = 0;

        float getEnvSample() {
            float y=0.f;
            switch (envState) {
                case Starting:
                    y = Window::raisedCosShort[envIdx];
                    incEnv();
                    break;;
                case Stopping:
                    y = Window::raisedCosShort[envIdx];
                    decEnv();
                    break;
                case Playing:
                    y = 1.0;
                    break;
                case Stopped:
                default:
                    y = 0.f;
            }
            return y;
        }

    private:
        void incEnv() {
            envIdx++;
            if (envIdx >= static_cast<int>(Window::raisedCosShortLen)) {
                envIdx = Window::raisedCosShortLen-1;
                envState = Playing;
            }
        }

        void decEnv() {
            envIdx--;
            if (envIdx < 0) {
                envIdx = 0;
                envState = Stopped;
                shouldStop = true;
                std::cerr << "Tape: fade-out finished; stopping" << std::endl;
            }
        }

    };

}


#endif //CRONE_SOUNDFILESTREAM_H
