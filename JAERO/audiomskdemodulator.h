#ifndef AUDIOMSKDEMODULATOR_H
#define AUDIOMSKDEMODULATOR_H

#include <QObject>
#include <QAudioInput>
#include <QMediaDevices>

#include "mskdemodulator.h"

class AudioMskDemodulator : public MskDemodulator
{
    Q_OBJECT
public:
    struct Settings : public MskDemodulator::Settings
    {
        QAudioDevice audio_device_in;
        double buffersizeinsecs;
        Settings()
        {
            audio_device_in=QMediaDevices::defaultAudioInput();
            buffersizeinsecs=1.0;
        }
    };
    explicit AudioMskDemodulator(QObject *parent = 0);
    ~AudioMskDemodulator();
    void start();
    void stop();
    void setSettings(Settings settings);
private:
    Settings settings;
    QAudioFormat m_format;
    QAudioInput *m_audioInput;
};

#endif // AUDIOMSKDEMODULATOR_H
