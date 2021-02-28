#include "daisysp.h"
#include "daisy_seed.h"
#include <algorithm>

// Shortening long macro for sample rate
#ifndef sample_rate

#endif

// Interleaved audio definitions
#define LEFT (i)
#define RIGHT (i + 1)

using namespace daisysp;
using namespace daisy;

static DaisySeed seed;

// Helper Modules
//static Metro tick;
static ReverbSc verb;
MidiHandler midi;

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
        env_.SetTime(ADSR_SEG_ATTACK, 0.25f);
        env_.SetTime(ADSR_SEG_DECAY, 0.005f);
        env_.SetTime(ADSR_SEG_RELEASE, 0.2f);
        filt_.Init(samplerate);
        filt_.SetFreq(6000.f);
        filt_.SetRes(0.6f);
        filt_.SetDrive(0.8f);
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
            filt_.Process(sig);
            return filt_.Low() * (velocity_ / 127.f) * amp;
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

    void SetCutoff(float val) { filt_.SetFreq(val); }

    inline bool  IsActive() const { return active_; }
    inline float GetNote() const { return note_; }

  private:
    Oscillator osc_;
    Svf        filt_;
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

    void SetCutoff(float all_val)
    {
        for(size_t i = 0; i < max_voices; i++)
        {
            voices[i].SetCutoff(all_val);
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

static VoiceManager<24> voice_handler;


static float trig = 0;

static void AudioCallback(float **in, float **out, size_t size)
{
    float sig_out;
    float sum = 0.f;
    // Assign Output Buffers
    float *out_left, *out_right;
    float dry, send, wetl, wetr; // Effects Vars
    out_left  = out[0];
    out_right = out[1];

    for(size_t i = 0; i < size; i++)
    {
        dry  = voice_handler.Process() * 0.5f; 
        send = dry * 0.45f;
        verb.Process(send, send, &wetl, &wetr);

        // Output
        out_left[i]  = dry + wetl;
        out_right[i] = dry + wetr;
        trig = 0.0;
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
    float init_buff[256]; // buffer for Pluck impulse

    // initialize seed hardware and daisysp modules
    float sample_rate;
    seed.Configure();
    seed.Init();
    sample_rate = seed.AudioSampleRate();
    midi.Init(MidiHandler::INPUT_MODE_UART1, MidiHandler::OUTPUT_MODE_NONE);

    verb.Init(sample_rate);
    verb.SetFeedback(0.95f);
    verb.SetLpFreq(5000.0f);

    voice_handler.Init(sample_rate);

    // start callback
    seed.StartAudio(AudioCallback);
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
