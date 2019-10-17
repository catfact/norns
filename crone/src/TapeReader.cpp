#include "TapeReader.h"

//------------------
//--- TapeReader
TapeReader::TapeReader() {
    actionQ = std::make_shared<std::queue<Action>>();
    ringBuf = std::shared_ptr<jack_ringbuffer_t>(jack_ringbuffer_create(ringBufBytes));
    diskWorker = std::make_unique<DiskWorker>(actionQ, ringBuf);
    audioWorker = std::make_unique<AudioWorker>(actionQ, ringBuf);
}

void TapeReader::processAudioBlock(float *dst[numChannels], size_t numFrames) {
    audioWorker->processAudioBlock(dst, numFrames);
}


//--------------------
//--- DiskWorker

//--------------------
//--- AudioWorker
void TapeReader::AudioWorker::processAudioBlock(float *dst[numChannels], size_t numFrames) {
    //...
}

void TapeReader::AudioWorker::pullAudio(size_t numFrames) {
}
