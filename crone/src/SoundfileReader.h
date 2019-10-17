//
// Created by emb on 10/13/19.
//

#ifndef CRONE_SOUNDFILEREADER_H
#define CRONE_SOUNDFILEREADER_H

#include "SoundfileStream.h"

namespace crone {

    template<int numChannels>
    class Reader : public SoundfileStream<numChannels, > {
    public:
        bool loopMode = true;
        bool pauseMode = true;
    private:
        typedef SoundfileStream<numChannels> Super;
        typedef typename Super::Sample Sample;
        size_t frames{};
        size_t framesBeforeFadeout{};
        size_t framesProcessed = 0;
        static constexpr size_t maxFramesToRead = Super::ringBufFrames;
        // interleaved buffer from soundfile (disk thread)
        Sample diskInBuf[Super::frameSize * maxFramesToRead]{};
        // buffer for deinterleaving after ringbuf (audio thread)
        Sample pullBuf[Super::frameSize * maxFramesToRead]{};
        std::atomic<bool> isPrimed{};
        bool needsData{};

    private:
//        // prime the ringbuffer
//        bool prime() {
//            jack_ringbuffer_t *rb = this->ringBuf.get();
//            size_t framesToRead = jack_ringbuffer_write_space(rb) / Super::frameSize;
//            if (framesToRead > maxFramesToRead) { framesToRead = maxFramesToRead; };
//            auto framesRead = (size_t) sf_readf_float(this->file, diskInBuf, framesToRead);
//            jack_ringbuffer_write(rb, (char *) diskInBuf, Super::frameSize * framesRead);
//
//            if (framesRead != framesToRead) {
//                std::cerr << "Tape::Reader: warning! priming not complete" << std::endl;
//                Super::shouldStop = true;
//                return false;
//            }
//
//            return true;
//        }

    public:
        // from audio thread
//        void process(float *dst[numChannels], size_t numFrames) {
//            if (!Super::isRunning || !isPrimed) {
//                for (size_t fr = 0; fr < numFrames; ++fr) {
//                    for (int ch = 0; ch < numChannels; ++ch) {
//                        dst[ch][fr] = 0.f;
//                    }
//                }
//                return;
//            }
//
//            jack_ringbuffer_t* rb = this->ringBuf.get();
//            auto framesInBuf = jack_ringbuffer_read_space(rb) / Super::frameSize;
//
//            //  if ringbuf isn't full enough, it's probably cause we're at EOF
//            if(framesInBuf < numFrames) {
//
//                // pull from ringbuffer
//                jack_ringbuffer_read(rb, (char*)pullBuf, framesInBuf * Super::frameSize);
//                float* src = pullBuf;
//                size_t fr = 0;
//                // de-interleave, apply amp, copy to output
//                while (fr < framesInBuf) {
//                    float amp = Super::getEnvSample();
//                    for (int ch = 0; ch < numChannels; ++ch) {
//                        dst[ch][fr] = *src++ * amp;
//                    }
//                    fr++;
//                }
//                // zero the remainder
//                while(fr < numFrames) {
//                    for (int ch = 0; ch < numChannels; ++ch) {
//                        dst[ch][fr] = 0.f;
//                    }
//                    fr++;
//                }
//                if (loopMode) {
//                    start();
//                } else {
//                    Super::isRunning = false;
//                }
//            } else {
//
//                // pull from ringbuffer
//                jack_ringbuffer_read(rb, (char *) pullBuf, numFrames * Super::frameSize);
//
//                if (this->mut.try_lock()) {
//                    this->needsData = true;
//                    this->cv.notify_one();
//                    this->mut.unlock();
//                }
//
//                float *src = pullBuf;
//
//                if (framesProcessed > (framesBeforeFadeout-numFrames)) {
//                    if (Super::envState != Super::EnvState::Stopping) {
//                        Super::envState = Super::EnvState::Stopping;
//                    }
//                }
//
//                // de-interleave, apply amp, copy to output
//                for (size_t fr = 0; fr < numFrames; ++fr) {
//                    float amp = Super::getEnvSample();
//                    for (int ch = 0; ch < numChannels; ++ch) {
//                        dst[ch][fr] = *src++ * amp;
//                    }
//                }
//
//                framesProcessed += numFrames;
//            }
//        }

        // from any thread
        bool open(const std::string &path) {
            SF_INFO sfInfo;

            if ((this->file = sf_open(path.c_str(), SFM_READ, &sfInfo)) == NULL) {
                char errstr[256];
                sf_error_str(0, errstr, sizeof(errstr) - 1);
                std::cerr << "cannot open sndfile" << path << " for output (" << errstr << ")" << std::endl;
                return false;
            }

            if (sfInfo.frames < 1) {

                std::cerr << "error reading file " << path << " (no frames available)" << std::endl;
                return false;
            }

            this->frames = static_cast<size_t>(sfInfo.frames);
            framesBeforeFadeout = this->frames - Window::raisedCosShortLen - 1;
            return this->frames > 0;
        }

        void start() override {
            if (pauseMode) {
            } else {
                framesProcessed = 0;
                jack_ringbuffer_reset(this->ringBuf.get());
                isPrimed = false;
                Super::start();
            }
        }

    private:
        // from disk thread
        void diskLoop() override {
            prime();
            isPrimed = true;
            Super::isRunning = true;
            Super::shouldStop = false;
            while (!Super::shouldStop) {
                {
                    std::unique_lock<std::mutex> lock(this->mut);
                    this->cv.wait(lock, [this] {
                        return this->needsData;
                    });
                    // check for spurious wakeup
                    if (!needsData) { continue; }
                }

                jack_ringbuffer_t *rb = this->ringBuf.get();

                size_t framesToRead = jack_ringbuffer_write_space(rb) / Super::frameSize;
                if (framesToRead < 1) {
                    {
                        std::unique_lock<std::mutex> lock(this->mut);
                        needsData = false;
                    }
                    continue;
                }

                if (framesToRead > maxFramesToRead) {
                    // _really_ shouldn't happen
                    framesToRead = maxFramesToRead;
                };
                auto framesRead = (size_t) sf_readf_float(this->file, diskInBuf, framesToRead);
                if (framesRead < framesToRead) {
                    std::cerr << "Tape::Reader::diskloop() read EOF" << std::endl;
                    Super::shouldStop = true;
                }

                jack_ringbuffer_write(rb, (char *) diskInBuf, Super::frameSize * framesRead);
                {
                    std::unique_lock<std::mutex> lock(this->mut);
                    needsData = false;
                }

            }
            sf_close(this->file);
            std::cerr << "Tape::reader closed file" << std::endl;
        }

    }; // Reader class

}

#endif //CRONE_SOUNDFILEREADER_H
