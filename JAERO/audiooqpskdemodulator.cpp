#include "audiooqpskdemodulator.h"

#include <QDebug>
#include <QMediaDevices>

AudioOqpskDemodulator::AudioOqpskDemodulator(QObject *parent)
    :   OqpskDemodulator(parent),
      m_audioInput(NULL)
{
    //
}

void AudioOqpskDemodulator::start()
{
    OqpskDemodulator::start();

    if(!settings.zmqAudio)
    {
        if(m_audioInput)m_audioInput->start(this);
    }

}

void AudioOqpskDemodulator::stop()
{
    if(m_audioInput)m_audioInput->stop();
    OqpskDemodulator::stop();
}

void AudioOqpskDemodulator::setSettings(Settings _settings)
{
    bool wasopen=isOpen();
    stop();

    //if Fs has changed or the audio device doesnt exist or the input device has changed then need to redo the audio device
    if((_settings.Fs!=settings.Fs)||(!m_audioInput)||(_settings.audio_device_in!=settings.audio_device_in))
    {
        settings=_settings;

        if(m_audioInput)m_audioInput->deleteLater();

        //set the format
        m_format.setSampleRate(settings.Fs);
        m_format.setChannelCount(1);
        // m_format.setSampleSize(16);
        // m_format.setCodec("audio/pcm");
        // m_format.setByteOrder(QAudioFormat::LittleEndian);
        m_format.setSampleFormat(QAudioFormat::Int16);

        //setup
        m_audioInput = new QAudioSink(settings.audio_device_in, m_format, this);
        // m_audioInput->setBufferSize(settings.Fs*settings.buffersizeinsecs);//buffersizeinsecs seconds of buffer
    }
    settings=_settings;

    OqpskDemodulator::setSettings(settings);

    if(wasopen)start();

}

AudioOqpskDemodulator::~AudioOqpskDemodulator()
{
    stop();
}
