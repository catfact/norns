#include <memory>
#include <queue>
#include <string>

#include <jack/types.h>
#include <jack/ringbuffer.h>

#include <sndfile.h>

class TapeReader {
public:
    //---------------
    //--- public API

    // constants, types
    typedef jack_default_audio_sample_t Sample;
    static constexpr int numChannels = 2;

    // functions
    void processAudioBlock(float *dst[numChannels], size_t numFrames);

    TapeReader();

protected:
    typedef enum {
        DiskOpen,
        DiskSeek,
        DiskClose,
        AudioPlay,
        AudioPause,
    } Action;

    // action FIFO queue
    typedef std::shared_ptr<std::queue<Action>> ActionQ;
    ActionQ actionQ;

    // ring buffer
    typedef std::shared_ptr<jack_ringbuffer_t> RingBuf;
    RingBuf ringBuf;


    // helper constants
    static constexpr size_t ringBufFrames = 16384;
    static constexpr size_t sampleSize = sizeof(Sample);
    static constexpr size_t frameSize = sampleSize * numChannels;
    static constexpr size_t ringBufBytes = frameSize * ringBufFrames;
 
    //-------------------
    //---- helper classes

    class DiskWorker {

        friend class TapeReader;

    private:
        static constexpr size_t maxFramesToRead = ringBufFrames;

        typedef enum {
            // Closed: no file, can't play
                    Closed,
            // Opening: opening a file
                    Opening,
            // Priming: filling the whole ringbuffer
                    Priming,
            // Waiting: nothing to do, ready to stream
                    Waiting,
            // Streaming: business as usual
                    Streaming,
            // Eof: we're out of samples, and nothing else is happening
                    Eof,
            // Seeking: seeking to a new position in the file
                    Seeking,
            // Closing: closing the file
                    Closing,
        } State;

	typedef enum {
	    Play,
	    Pause
	} Action;

        std::string filePath;
	boost::spsc_lockfree;
        RingBuf ringBuf;

        void checkQ();

    protected:
        void perform();
    public:
        DiskWorker(ActionQ actQ, RingBuf rBuf) : actionQ(actQ), ringBuf(rBuf) {}
    };


    class AudioWorker {

        friend class TapeReader;

        typedef enum {
            // Waiting: data isn't ready. expecting to be Starting
                    Waiting,
            // Starting: opening the envelope, expecing to be Playing
                    Starting,
            // we're streaming along
                    Playing,
            // Pausing: closing the envelope, expecting to be Paused
                    Pausing,
            // Paused: zero output, expecting to be Resuming
                    Paused,
            // Resuming: opening envelope, expecting to be Playing (no different from Starting)
                    Resuming,
            // Stopped: zero output, no expectations
                    Stopped,
            // Looping: closing the envelope, expecting to be Waiting
                    Looping,
        } State;

    public:
        AudioWorker(ActionQ actQ, RingBuf rBuf) : actionQ(actQ), ringBuf(rBuf) {}
    protected:
        void processAudioBlock(float *dst[numChannels], size_t numFrames);
    private:     
        void checkQ();
	void pullAudio(size_t numFrames);
	   
    private:
	
        ActionQ actionQ;
        RingBuf ringBuf;

    };

private:
    std::unique_ptr<DiskWorker> diskWorker;
    std::unique_ptr<AudioWorker> audioWorker;

};
