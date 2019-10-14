//
// Created by emb on 12/01/18.
//

#ifndef CRONE_TAPE_H
#define CRONE_TAPE_H

#include "SoundfileReader.h"
#include "SoundfileWriter.h"

namespace crone {

    template<int numChannels>
    class Tape {

    public:
        Writer<numChannels> writer;
        Reader<numChannels> reader;

    public:
        bool isWriting() { return writer.isRunning; }
        bool isReading() { return reader.isRunning; }
    };

}

#endif // CRONE_TAPE_H
