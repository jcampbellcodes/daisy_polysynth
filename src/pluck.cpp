#include "daisysp.h"
#include "daisy_seed.h"
#include "cap1188.h"
#include <algorithm>
#include <array>

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
        osc_.SetAmp(0.3f);
        osc_.SetWaveform(Oscillator::WAVE_SIN);

        env_.Init(samplerate);
        env_.SetSustainLevel(0.5f);
        env_.SetTime(ADSR_SEG_ATTACK, 1.01f);
        env_.SetTime(ADSR_SEG_DECAY, 0.005f);
        env_.SetTime(ADSR_SEG_RELEASE, 0.2f);
        filt_.Init(samplerate);
        filt_.SetFreq(6000.f);
        filt_.SetRes(0.6f);
        filt_.SetDrive(0.3f);

        lfo_.Init(samplerate);
        lfo_.SetAmp(1.0f);
        lfo_.SetFreq(0.1);
    }

    float Process()
    {
        if(active_)
        {
            float sig, amp;
            amp = env_.Process(env_gate_);
            if(!env_.IsRunning())
                active_ = false;
            
            float note = use_lfo_ ? note_ + lfo_.Process() : note_;
            osc_.SetFreq(mtof(note));
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
        use_lfo_ = note != 69;
    }

    void OnNoteOff() { env_gate_ = false; }

    void SetCutoff(float val) { filt_.SetFreq(val); }

    inline bool  IsActive() const { return active_; }
    inline float GetNote() const { return note_; }

  private:
    Oscillator osc_;
    Oscillator lfo_;
    Svf        filt_;
    Adsr       env_;
    float      note_, velocity_;
    bool       active_;
    bool       env_gate_;
    bool       use_lfo_;
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

static DaisySeed seed_handle;
static ReverbSc verb;
MidiUartHandler midi;

// configure max voices here
static VoiceManager<18> voice_handler;

static void AudioCallback(const float * const*inBuffer, float **outBuffer, unsigned int inNumSamples)
{
    // Assign Output Buffers
    float *out_left = outBuffer[0];
    float *out_right = outBuffer[1];
    float dry = 0.0f, send = 0.0f, wetl = 0.0f, wetr = 0.0f; // Effects Vars
    for(size_t sample = 0; sample < inNumSamples; sample++)
    {
        // get dry sample from the state of the voices
        dry  = voice_handler.Process() * 0.5f; 
        // run an attenuated dry signal through the reverb
        send = dry * 0.45f;
        verb.Process(send, send, &wetl, &wetr);

        // sum the dry oscillator and processed reverb signal
        float outL = dry + wetl;
        float outR = dry + wetr;
        out_left[sample] = outL;
        out_right[sample] = outR;
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

void HandleNoteOn(NoteOnEvent p)
{
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

void HandleNoteOff(NoteOffEvent p)
{
    voice_handler.OnNoteOff(p.note, p.velocity);
}

int main(void)
{
    // initialize seed hardware and daisysp modules
    float sample_rate;
    seed_handle.Configure();
    seed_handle.Init();
    seed_handle.SetAudioBlockSize(1);
    seed_handle.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    sample_rate = seed_handle.AudioSampleRate();
    MidiUartHandler::Config midi_config;
    midi.Init(midi_config);

    verb.Init(sample_rate);
    verb.SetFeedback(0.85f);
    verb.SetLpFreq(5000.0f);

    voice_handler.Init(sample_rate);

    // start callback
    seed_handle.StartAudio(AudioCallback);
    midi.StartReceive();

    cap1188 cap_touch;
    while(!cap_touch.begin()) System::Delay(1);

    constexpr size_t numLeaves = 6;
    struct notes_t
    {
        int32_t lowerNote = 0;
        int32_t lowerVelocity = 0;
        int32_t upperNote = 0;
        int32_t upperVelocity = 0;
    };

    // F4, A4, F5, B5, D6, F6
    std::array<notes_t, numLeaves> notes = {notes_t{65, 32, 0, 0}, notes_t{69, 32, 0, 0}, notes_t{77, 32, 0, 0},
                                            notes_t{83, 64, 85, 20}, notes_t{86, 64, 88, 44}, notes_t{89, 64, 91, 20}};
    std::array<int32_t, numLeaves> pins = {28, 27, 26, 25, 24, 23};
    std::array<Switch, numLeaves> buttons;
    std::array<bool, numLeaves> states;

    for(auto idx = 0; idx < notes.size(); idx++)
    {
        Switch adc;
        adc.Init(seed_handle.GetPin(pins[idx]), 1000);
        buttons[idx] = adc;
        states[idx] = false;
    }

    while(1) 
    {
        for(auto idx = 0; idx < buttons.size(); idx++)
        {
            buttons[idx].Debounce();
            if(!buttons[idx].Pressed())
            {
                if(!states[idx])
                {
                    states[idx]=true;
                    NoteOnEvent event;
                    event.channel = 0;
                    event.velocity = notes[idx].lowerNote;
                    event.note = notes[idx].lowerNote;
                    HandleNoteOn(event);
                    if(notes[idx].upperNote)
                    {
                        event.note = notes[idx].upperNote;
                        event.velocity = notes[idx].upperVelocity;
                        HandleNoteOn(event);
                    }
                }
            }
            else
            {
                if(states[idx])
                {
                    states[idx]=false;
                    NoteOffEvent event;
                    event.channel = 0;
                    event.velocity = 0.0;
                    event.note = notes[idx].lowerNote;
                    HandleNoteOff(event);
                    if(notes[idx].upperNote)
                    {
                        event.note = notes[idx].upperNote;
                        HandleNoteOff(event);
                    }
                }
            }
        }
        seed_handle.SetLed(buttons[0].Pressed());
        System::Delay(7);
    }
}
