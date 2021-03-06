#include <cstdlib>
#include <iostream>

#include <math.h>

#include "defjams.h"
#include "mixer.h"


Mixer::Mixer() : bpm(80), microtick(0), stream(0)
{
    for ( int i = 0; i < 5; i++)
        envelopes.push_back(new Envelope());
    //sleeptime = (60000 / bpm) / 16;
}

bool Mixer::open(PaDeviceIndex index)
{
    PaStreamParameters outputParameters;

    outputParameters.device = index;
    if (outputParameters.device == paNoDevice) {
        return false;
    }

    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;


    PaError err = Pa_OpenStream(
        &stream,
        NULL, /* no input */
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,      /* we won't output out of range samples so don't bother clipping them */
        &Mixer::processAudio,
        this            /* Using 'this' for userData so we can cast to Mixer* in paCallback method */
        );

    if (err != paNoError)
    {
        /* Failed to open stream to device !!! */
        return false;
    }

    err = Pa_SetStreamFinishedCallback( stream, &Mixer::paStreamFinished );

    if (err != paNoError)
    {
        Pa_CloseStream( stream );
        stream = 0;

        return false;
    }

    return true;
}

bool Mixer::close()
{
    if (stream == 0)
        return false;

    PaError err = Pa_CloseStream( stream );
    stream = 0;

    return (err == paNoError);
}


bool Mixer::start()
{
    if (stream == 0)
        return false;

    PaError err = Pa_StartStream( stream );

    return (err == paNoError);
}

int Mixer::sigSize()
{
    return signals.size();
}

bool Mixer::stop()
{
    if (stream == 0)
        return false;

    PaError err = Pa_StopStream( stream );

    return (err == paNoError);
}

void Mixer::goMix()
{
    if (Mixer::open(Pa_GetDefaultOutputDevice()))
        Mixer::start();
}

/* The instance callback, where we have access to every method/variable in object of class Mixer */
int Mixer::paCallbackMethod(const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags)
{
    float *out = (float*)outputBuffer;
    unsigned long i, j;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;

    //float outval = 0;
    //float ns = 0;

    if (signals.size() == 0)
    {
        for( i=0; i<framesPerBuffer; i++ )
        {
            *out++ = 0;
            *out++ = 0;
        }
    }
    else
    {
        for( i=0; i<framesPerBuffer; i++ )
        {
            float outval = 0;
            float ns = 0;
            float mod = 0;
            for ( j=0; j<signals.size(); j++ )
            {
                if (signals[j]->getVol() > 0.0) {
                  ns = signals[j]->genNextSound();
                  if (j < envelopes.size()) {
                      //std::cout << "Appplying envelope\n";
                      mod = envelopes[j]->genNextVal();
                      ns *= mod;
                  }

                  outval += ns;
                }
            }

            //*out++ = outval / signals.size();
            //*out++ = outval / signals.size();
            if (outval > 1.0)
                outval = 1.0;
            if (outval < -1.0)
                outval = -1.0;
            //printf("OUTVAL: %f\n", outval);
            //if (outval > 1.0 || outval < -1.0) {
                //*out++ = outval / signals.size();
                //*out++ = outval / signals.size();
            //} else {
            *out++ = outval;
            *out++ = outval;
            //}
        }
    }

    return paContinue;

}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
int Mixer::processAudio( const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData )
{
    /* Here we cast userData to Mixer* type so we can call the instance method paCallbackMethod, we can do that since 
       we called Pa_OpenStream with 'this' for userData */
    return ((Mixer*)userData)->paCallbackMethod(inputBuffer, outputBuffer,
        framesPerBuffer,
        timeInfo,
        statusFlags);
}


void Mixer::paStreamFinishedMethod()
{
    printf( "Stream Completed\n");
}

void Mixer::paStreamFinished(void* userData)
{
    return ((Mixer*)userData)->paStreamFinishedMethod();
}
