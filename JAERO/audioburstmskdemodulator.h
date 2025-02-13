#ifndef AUDIOBURSTMSKDEMODULATOR_H
#define AUDIOBURSTMSKDEMODULATOR_H

#include <QObject>
#include <QAudioInput>
#include <QMediaDevices>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioSink>

#include "burstmskdemodulator.h"

class AudioBurstMskDemodulator : public BurstMskDemodulator
{
    Q_OBJECT
public:
    struct Settings : public BurstMskDemodulator::Settings
    {
        QAudioDevice audio_device_in;
        double buffersizeinsecs;
        Settings()
        {
            audio_device_in=QMediaDevices::defaultAudioInput();
            buffersizeinsecs=1.0;
        }
    };
    explicit AudioBurstMskDemodulator(QObject *parent = 0);
    ~AudioBurstMskDemodulator();
    void start();
    void stop();
    void setSettings(Settings settings);
private:
    Settings settings;
    QAudioFormat m_format;
    QAudioSink *m_audioInput;
};

#endif // AUDIOBURSTMSKDEMODULATOR_H
