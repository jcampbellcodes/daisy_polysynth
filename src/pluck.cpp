#include "daisysp.h"
#include "daisy_seed.h"
#include <algorithm>

using namespace daisysp;
using namespace daisy;
using namespace daisy::seed;

class Voice
{
  public:
    Voice() {}
    ~Voice() {}
    void Init(float samplerate)
    {
        active_ = false;
        osc_.Init(samplerate);
        osc_.SetAmp(0.75f);
        osc_.SetWaveform(Oscillator::WAVE_POLYBLEP_TRI);
        env_.Init(samplerate);
        env_.SetSustainLevel(0.5f);
        env_.SetTime(ADSR_SEG_ATTACK, 1.01f);
        env_.SetTime(ADSR_SEG_DECAY, 0.005f);
        env_.SetTime(ADSR_SEG_RELEASE, 0.2f);
    }

    float Process()
    {
        if(active_)
        {
            float sig, amp;
            amp = env_.Process(env_gate_);
            if(!env_.IsRunning())
                active_ = false;
            sig = osc_.Process();
            return sig * (velocity_ / 127.f) * amp;
        }
        return 0.f;
    }

    void OnNoteOn(float note, float velocity)
    {
        note_     = note;
        velocity_ = velocity;
        osc_.SetFreq(mtof(note_));
        active_   = true;
        env_gate_ = true;
    }

    void OnNoteOff() { env_gate_ = false; }

    inline bool  IsActive() const { return active_; }
    inline float GetNote() const { return note_; }

  private:
    Oscillator osc_;
    Adsr       env_;
    float      note_, velocity_;
    bool       active_;
    bool       env_gate_;
};

template <size_t max_voices>
class VoiceManager
{
  public:
    VoiceManager() {}
    ~VoiceManager() {}

    void Init(float samplerate)
    {
        for(size_t i = 0; i < max_voices; i++)
        {
            voices[i].Init(samplerate);
        }
    }

    float Process()
    {
        float sum;
        sum = 0.f;
        for(size_t i = 0; i < max_voices; i++)
        {
            sum += voices[i].Process();
        }
        return sum;
    }

    void OnNoteOn(float notenumber, float velocity)
    {
        Voice *v = FindFreeVoice();
        if(v == NULL)
            return;
        v->OnNoteOn(notenumber, velocity);
    }

    void OnNoteOff(float notenumber, float velocity)
    {
        for(size_t i = 0; i < max_voices; i++)
        {
            Voice *v = &voices[i];
            if(v->IsActive() && v->GetNote() == notenumber)
            {
                v->OnNoteOff();
            }
        }
    }

    void FreeAllVoices()
    {
        for(size_t i = 0; i < max_voices; i++)
        {
            voices[i].OnNoteOff();
        }
    }

  private:
    Voice  voices[max_voices];
    Voice *FindFreeVoice()
    {
        Voice *v = NULL;
        for(size_t i = 0; i < max_voices; i++)
        {
            if(!voices[i].IsActive())
            {
                v = &voices[i];
                break;
            }
        }
        return v;
    }
};

static DaisySeed seed_handle;
static ReverbSc verb;
static Svf      filt;
MidiUartHandler midi;
static VoiceManager<24> voice_handler;

static void AudioCallback(const float * const*inBuffer, float **outBuffer, unsigned int inNumSamples)
{
    // Assign Output Buffers
    float *out_left = outBuffer[0];
    float *out_right = outBuffer[1];
    float dry = 0.0f, send = 0.0f, wetl = 0.0f, wetr = 0.0f; // Effects Vars
    for(size_t sample = 0; sample < inNumSamples; sample++)
    {
        filt.Process(voice_handler.Process());
        // get dry sample from the state of the voices
        dry  = filt.Low() * 0.5f; 
        // run an attenuated dry signal through the reverb
        send = dry * 0.45f;
        verb.Process(send, send, &wetl, &wetr);
        // sum the dry oscillator and processed reverb signal
        out_left[sample]  = dry + wetl;
        out_right[sample] = dry + wetr;
    }
}

// Typical Switch case for Message Type.
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            // Note Off can come in as Note On w/ 0 Velocity
            if(p.velocity == 0.f)
            {
                voice_handler.OnNoteOff(p.note, p.velocity);
            }
            else
            {
                voice_handler.OnNoteOn(p.note, p.velocity);
            }
        }
        break;
        case NoteOff:
        {
            NoteOnEvent p = m.AsNoteOn();
            voice_handler.OnNoteOff(p.note, p.velocity);
        }
        break;
        default: break;
    }
}

int main(void)
{
    // initialize seed hardware and daisysp modules
    float sample_rate;
    seed_handle.Configure();
    seed_handle.Init();
    sample_rate = seed_handle.AudioSampleRate();
    MidiUartHandler::Config midi_config;
    midi.Init(midi_config);

    filt.Init(sample_rate);
    filt.SetFreq(6000.f);
    filt.SetRes(0.6f);
    filt.SetDrive(0.8f);

    verb.Init(sample_rate);
    verb.SetFeedback(0.95f);
    verb.SetLpFreq(5000.0f);

    voice_handler.Init(sample_rate);

    // start callback
    seed_handle.StartAudio(AudioCallback);
    midi.StartReceive();

    while(1) 
    {
        midi.Listen();
        while(midi.HasEvents())
        {
            HandleMidiMessage(midi.PopEvent());
        }
    }
}
