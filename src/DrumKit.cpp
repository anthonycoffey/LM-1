#include "DrumKit.h"
#include "DrumSynth.h"

#if LMONE_HAS_BINARY_KIT
 #include "BinaryData.h"
#endif

//==============================================================================
DrumKit::DrumKit()
{
    for (int i = 0; i < kNumVoices; ++i)
        voices[(size_t) i].name = LMOne::kVoiceDefs[(size_t) i].name;
}

//==============================================================================
namespace
{
    // Largest sample we'll accept (guards against loading an absurd file into RAM).
    constexpr juce::int64 kMaxSampleFrames = (juce::int64) 30 * 192000; // 30 s @ 192 kHz

    // Procedural fallback per voice index — used until real WAVs are supplied.
    // Crude on purpose; these get replaced by the bundled/loaded samples.
    juce::AudioBuffer<float> makeProcedural (int voiceIndex, double sr)
    {
        using namespace DrumSynth;
        switch (voiceIndex)
        {
            case 0:  return makeKick    (sr);            // Bass
            case 1:  return makeSnare   (sr);            // Snare
            case 2:  return makeHat     (sr, 0.06, 60.0);// Hi-Hat (closed-ish)
            case 3:  return makeHat     (sr, 0.12, 28.0);// Cabasa  (shaker placeholder)
            case 4:  return makeHat     (sr, 0.20, 12.0);// Tambourine (placeholder)
            case 5:  return makeTom     (sr, 100.0);     // Tom Lo
            case 6:  return makeTom     (sr, 160.0);     // Tom Hi
            case 7:  return makeTom     (sr, 200.0);     // Conga Lo
            case 8:  return makeTom     (sr, 280.0);     // Conga Hi
            case 9:  return makeCowbell (sr);            // Cowbell
            case 10: return makeTom     (sr, 1200.0);    // Clave (placeholder)
            case 11: return makeClap    (sr);            // Clap
            case 12: return makeHat     (sr, 0.40, 9.0); // Hi-Hat Open (placeholder)
            default: return makeKick    (sr);
        }
    }

    // Read a decoded reader into a voice's buffer (mono downmix). Only touches the
    // voice on success, so a failed read leaves the existing sample in place.
    bool readerToVoice (juce::AudioFormatReader* reader, DrumKit::VoiceSample& vs)
    {
        if (reader == nullptr)
            return false;

        const juce::int64 len64 = reader->lengthInSamples;
        if (len64 <= 0 || len64 > kMaxSampleFrames)
            return false;

        const int len   = (int) len64;
        const int srcCh = juce::jmax (1, (int) reader->numChannels);

        juce::AudioBuffer<float> tmp (srcCh, len);
        reader->read (&tmp, 0, len, 0, true, true);

        juce::AudioBuffer<float> mono (1, len);
        mono.clear();
        for (int ch = 0; ch < srcCh; ++ch)
            mono.addFrom (0, 0, tmp, ch, 0, len, 1.0f / (float) srcCh);

        vs.buffer           = std::move (mono);
        vs.sourceSampleRate = reader->sampleRate;
        return true;
    }

#if LMONE_HAS_BINARY_KIT
    // Lower-case, alphanumeric-only form of a string, for forgiving filename matching.
    juce::String normalize (const juce::String& s)
    {
        return s.toLowerCase().retainCharacters ("abcdefghijklmnopqrstuvwxyz0123456789");
    }

    // Accepted (normalized) filename stems per voice — see assets/factory_kit/README.md.
    const std::array<juce::StringArray, 13> kVoiceAliases = { {
        { "bass", "kick", "bassdrum", "bd" },
        { "snare", "sd" },
        { "hihat", "hat", "hh", "hihatclosed", "closedhat" },
        { "cabasa" },
        { "tambourine", "tamb" },
        { "tomlo", "tom1", "lowtom", "tomlow" },
        { "tomhi", "tom2", "hightom", "tomhigh" },
        { "congalo", "conga1", "lowconga", "congalow" },
        { "congahi", "conga2", "highconga", "congahigh" },
        { "cowbell", "cow" },
        { "clave", "claves" },
        { "clap", "handclap", "clp" },
        { "hatopen", "hihatopen", "openhat", "ohh", "oh" },
    } };

    bool loadVoiceFromBinary (DrumKit::VoiceSample& vs, int voiceIndex, juce::AudioFormatManager& fm)
    {
        for (int j = 0; j < BinaryData::namedResourceListSize; ++j)
        {
            // originalFilenames hold bare names (e.g. "Bass.wav") — strip the
            // extension via string ops (juce::File would assert on a relative path).
            const juce::String fileName (BinaryData::originalFilenames[j]);
            const auto stem = normalize (fileName.upToLastOccurrenceOf (".", false, false));

            bool match = false;
            for (const auto& kw : kVoiceAliases[(size_t) voiceIndex])
                if (stem == kw) { match = true; break; }
            if (! match)
                continue;

            int size = 0;
            if (const char* data = BinaryData::getNamedResource (BinaryData::namedResourceList[j], size))
            {
                auto stream = std::make_unique<juce::MemoryInputStream> (data, (size_t) size, false);
                std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (std::move (stream)));
                if (readerToVoice (reader.get(), vs))
                {
                    vs.isProcedural = false;
                    return true;
                }
            }
        }
        return false;
    }
#endif
}

//==============================================================================
DrumKit::Ptr KitFactory::buildFactoryKit (double proceduralRate)
{
    DrumKit::Ptr kit = new DrumKit();

   #if LMONE_HAS_BINARY_KIT
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
   #endif

    for (int i = 0; i < DrumKit::kNumVoices; ++i)
    {
        auto& vs = kit->voice (i);
        vs.sourceTag   = "factory";
        vs.sourcePath  = {};
        vs.startSample = 0;
        vs.endSample   = -1;

        bool loaded = false;
       #if LMONE_HAS_BINARY_KIT
        loaded = loadVoiceFromBinary (vs, i, fm);
       #endif

        if (! loaded)
        {
            vs.buffer           = makeProcedural (i, proceduralRate);
            vs.sourceSampleRate = proceduralRate;
            vs.isProcedural     = true;
        }
        // name was set by DrumKit's constructor from kVoiceDefs.
    }

    return kit;
}

//==============================================================================
bool KitFactory::loadVoiceFromFile (DrumKit& kit, int voiceIndex, const juce::File& file)
{
    if (voiceIndex < 0 || voiceIndex >= DrumKit::kNumVoices)
        return false;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();               // WAV, AIFF, and friends

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    auto& vs = kit.voice (voiceIndex);
    if (! readerToVoice (reader.get(), vs))
        return false;                                   // unreadable -> leave voice untouched

    vs.sourceTag    = "file";
    vs.sourcePath   = file.getFullPathName();
    vs.startSample  = 0;
    vs.endSample    = -1;
    vs.isProcedural = false;
    // keep vs.name (the slot's instrument name)

    return true;
}
