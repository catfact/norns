//
// Created by emb on 10/13/19.
//

#ifndef CRONE_SOUNDFILEWRITER_H
#define CRONE_SOUNDFILEWRITER_H

#include "SoundfileStream.h"

namespace crone {

    template<int numChannels>
    class Writer : public SoundfileStream<numChannels> {
    private:
        typedef SoundfileStream<numChannels> Super;
        static constexpr size_t maxFramesToWrite = Super::ringBufFrames;
        static constexpr size_t minBytesToWrite = 2048; // totally arbitrary
        static constexpr size_t minFramesToWrite = minBytesToWrite * Super::frameSize;
        bool dataPending;
        //  buffer for writing to soundfile (disk thread)
        typename Super::Sample diskOutBuf[maxFramesToWrite * numChannels];
        //  buffer for interleaving before ringbuf (audio thread)
        typename Super::Sample pushBuf[maxFramesToWrite * numChannels];
        size_t numFramesCaptured;
        size_t maxFrames;

    public:
        // call from audio thread
        void process(const float *src[numChannels], size_t numFrames) {
            if (!Super::isRunning) { return; }

            // push to ringbuffer
            jack_ringbuffer_t *rb = this->ringBuf.get();
            size_t bytesToPush = numFrames * Super::frameSize;
            const size_t bytesAvailable = jack_ringbuffer_write_space(rb);

            if (bytesToPush > bytesAvailable) {
#if 0
                std::cerr << "Tape: writer overrun: "
                                  << bytesAvailable << " bytes available; "
                                  << bytesToPush << " bytes to push; "
                                  << numFramesCaptured << " frames captured"
                                  << std::endl;
#endif
                // discard input if the ringbuffer is full;
                // this causes a dropout but hopefully nothing Really Bad
                numFrames = bytesAvailable / Super::frameSize;
                bytesToPush = numFrames * Super::frameSize;
            }

            /// libsndfile requires interleaved data. we do that here before pushing to ringbuf
            float *dst = pushBuf;
            for (size_t fr = 0; fr < numFrames; ++fr) {
                // while we're interleaving, also apply envelope
                float amp = Super::getEnvSample();
                for (int ch = 0; ch < numChannels; ++ch) {

                    *dst++ = src[ch][fr] * amp;
                }
            }
            jack_ringbuffer_write(rb, (const char *) pushBuf, bytesToPush);
            this->dataPending = true;
            this->cv.notify_one();

            if (this->mut.try_lock()) {
                this->dataPending = true;
                this->cv.notify_one();
                this->mut.unlock();
            }
        }

        // call from disk thread
        void diskLoop() override {
            Super::isRunning = true;
            Super::shouldStop = false;
            numFramesCaptured = 0;
            size_t bytesAvailable;
            while (!Super::shouldStop) {
                {
                    std::unique_lock<std::mutex> lock(this->mut);
                    this->cv.wait(lock, [this] {
                        return this->dataPending;
                    });

                    // check for spurious wakeup
                    if (!dataPending) {
                        continue;
                    }
                }

                bytesAvailable = jack_ringbuffer_read_space(this->ringBuf.get());
                if (bytesAvailable < minBytesToWrite) {
                    {
                        std::unique_lock<std::mutex> lock(this->mut);
                        dataPending = false;
                    }
                    continue;
                }

                int framesToWrite = bytesAvailable / Super::frameSize;

                if (framesToWrite > (int) maxFramesToWrite) {
                    // _really_ shouldn't happen
                    std::cerr << "warning: SoundfileStream::Writer has too many frames to write" << std::endl;
                    framesToWrite = (int) maxFramesToWrite;
                }

                jack_ringbuffer_read(this->ringBuf.get(), (char *) diskOutBuf, framesToWrite * Super::frameSize);
                // immediately signal audio thread that we're done with pending data
                {
                    std::unique_lock<std::mutex> lock(this->mut);
                    dataPending = false;
                }

                if (sf_writef_float(this->file, diskOutBuf, framesToWrite) != framesToWrite) {
                    char errstr[256];
                    sf_error_str(nullptr, errstr, sizeof(errstr) - 1);
                    std::cerr << "error: SoundfileStream::writer failed to write (libsndfile: " << errstr << ")"
                              << std::endl;
                    this->status = EIO;
                    break;
                }

                numFramesCaptured += framesToWrite;
                if (numFramesCaptured >= maxFrames) {
                    std::cerr << "SoundfileStream::writer exceeded max frame count; aborting.";
                    break;
                }

            }

            std::cerr << "SoundfileStream::writer closing file...";
            sf_close(this->file);
            std::cerr << " done." << std::endl;
            Super::isRunning = false;
        }

        // from any thread
        bool open(const std::string &path,
                  size_t argMaxFrames = JACK_MAX_FRAMES, // <-- ridiculous big number
                  int sampleRate = 48000,
                  int bitDepth = 24) {
            SF_INFO sf_info;
            int short_mask;

            sf_info.samplerate = sampleRate;
            sf_info.channels = numChannels;

            switch (bitDepth) {
                case 8:
                    short_mask = SF_FORMAT_PCM_U8;
                    break;
                case 16:
                    short_mask = SF_FORMAT_PCM_16;
                    break;
                case 24:
                    short_mask = SF_FORMAT_PCM_24;
                    break;
                case 32:
                    short_mask = SF_FORMAT_PCM_32;
                    break;
                default:
                    short_mask = SF_FORMAT_PCM_24;
                    break;
            }
            sf_info.format = SF_FORMAT_WAV | short_mask;

            if ((this->file = sf_open(path.c_str(), SFM_WRITE, &sf_info)) == NULL) {
                char errstr[256];
                sf_error_str(nullptr, errstr, sizeof(errstr) - 1);
                std::cerr << "cannot open sndfile" << path << " for output (" << errstr << ")" << std::endl;
                return false;
            }

            // enable clipping during float->int conversion
            sf_command(this->file, SFC_SET_CLIPPING, NULL, SF_TRUE);

            this->maxFrames = argMaxFrames;
            jack_ringbuffer_reset(this->ringBuf.get());
            dataPending = false;
            return true;
        }

        Writer() : Super(),
                   dataPending(false),
                   numFramesCaptured(0),
                   maxFrames(JACK_MAX_FRAMES) {}
    }; // Writer class
}

#endif //CRONE_SOUNDFILEWRITER_H

