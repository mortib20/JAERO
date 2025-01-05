#ifndef AUDIOOQPSKDEMODULATOR_H
#define AUDIOOQPSKDEMODULATOR_H

#include <QObject>
#include <QAudioSource>
#include <QMediaDevices>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioSink>

#include "oqpskdemodulator.h"

class AudioOqpskDemodulator : public OqpskDemodulator
{
    Q_OBJECT
public:
    struct Settings : public OqpskDemodulator::Settings
    {
        QAudioDevice audio_device_in;
        double buffersizeinsecs;
        Settings()
        {
            audio_device_in=QMediaDevices::defaultAudioInput();
            buffersizeinsecs=1.0;
        }
    };
    explicit AudioOqpskDemodulator(QObject *parent = 0);
    ~AudioOqpskDemodulator();
    void start();
    void stop();
    void setSettings(Settings settings);
private:
    Settings settings;
    QAudioFormat m_format;
    QAudioSink *m_audioInput;
};

#endif // AUDIOOQPSKDEMODULATOR_H
