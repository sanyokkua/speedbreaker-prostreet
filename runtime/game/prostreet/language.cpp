// SpeedBreaker runtime. GPL-3.0-or-later (see COPYING).
//
// NFS: ProStreet: Settings > Game > Language (game/language.h, ProStreet's
// table). The USA disc ("En,Es") holds the text of 15 languages
// (LANGUAGES\<NAME>_{GLOBAL,FRONTEND,INGAME}.BIN), but its boot picks only
// English or Mexican Spanish from the console's language:
//   0x824372B4  bl XGetLanguage (sub_82A66D58); r3 = (== 5) ? 10 : 0
//   0x824372C8  bl SetCurrentLanguage (sub_824379D8), returning to 0x824372CC
// That call gets the chosen language; English (the default) keeps the game's
// choice. The boot sizes its language memory for the largest language, so
// any fits. Movies are always English (sub_823F0088 formats a fixed
// "_english"); subtitles follow the language by name. Speech:
// sub_822E4C08 asks the attribute lookup sub_8234D918 for collection
// 0xC243117C, record 1 French, 2 German, 3 Italian, 4 Spanish, 5 Japanese,
// 6 Russian, else 0 (English); the disc has only English's bank
// (SOUND\SPEECH\PASPEECH_EN), so a missing bank's record becomes English's,
// as in Most Wanted (where such a language had no speech at all).
#include <stdafx.h>
#include <game/language.h>

#include <kernel/function.h>
#include <user/paths.h>
#include <user/settings.h>

#include <filesystem>
#include <fstream>
#include <iterator>

extern "C" PPC_FUNC(__imp__sub_824379D8);  // SetCurrentLanguage(r3)
extern "C" PPC_FUNC(__imp__sub_8234D918);  // attribute lookup(r3, r4 collection, r5 record)

namespace
{
    using namespace game::language;

    constexpr uint32_t kPsBootCall = 0x824372CC;
    constexpr uint32_t kPsSpeechPickBegin = 0x822E4C08, kPsSpeechPickEnd = 0x822E4FA0;
    constexpr uint32_t kPsSpeechCollection = 0xC243117C;
    // sub_822E4C08's records: the speech bank each asks for.
    constexpr const char* kSpeechCode[] = { "EN", "FR", "GE", "IT", "SP", "JA", "RU" };

    std::filesystem::path Child(const std::filesystem::path& dir, std::string_view name)
    {
        std::error_code ec;
        for (const auto& e : std::filesystem::directory_iterator(dir, ec))
        {
            const std::string n = e.path().filename().string();
            if (n.size() == name.size() && EndsWithNoCase(n, name))
                return e.path();
        }
        return {};
    }

    const std::vector<uint8_t>& Zdir()
    {
        static const std::vector<uint8_t> zdir = [] {
            std::vector<uint8_t> out;
            if (auto nfs = Child(GetGamePath(), "nfs"); !nfs.empty())
                if (auto index = Child(nfs, "zdir.bin"); !index.empty())
                {
                    std::ifstream f(index, std::ios::binary);
                    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
                }
            return out;
        }();
        return zdir;
    }

    bool HasText(const Language& l)
    {
        for (const char* part : { "GLOBAL", "FRONTEND", "INGAME" })
            if (!InArchive(Zdir(), std::string("LANGUAGES\\") + l.disc + "_" + part + ".BIN"))
                return false;
        return true;
    }

    // Which records' banks are installed (once; the lookup is hot).
    const std::array<bool, std::size(kSpeechCode)>& SpeechBanks()
    {
        static const auto banks = [] {
            std::array<bool, std::size(kSpeechCode)> out{};
            for (size_t i = 0; i < out.size(); i++)
                out[i] = InArchive(Zdir(), std::string("SOUND\\SPEECH\\PASPEECH_") + kSpeechCode[i] + ".CSI");
            return out;
        }();
        return banks;
    }
}

namespace game::language
{
    std::string MenuNote(int32_t id)
    {
        const Language* l = Find(id);
        if (!l || id == kEnglish)
            return std::string();
        if (!HasText(*l))
            return std::string("The installed files have no ") + l->name + " text: the game stays in English.";
        return std::string("Menus and race text in ") + l->name + ". Movies and speech in English.";
    }

    void SetLaunchLanguage(int32_t) {}  // no PAL release
}

PPC_FUNC(sub_824379D8)
{
    if (uint32_t(ctx.lr) == kPsBootCall)
    {
        const int32_t chosen = settings::GetInt(settings::Id::GameLanguage);
        if (const Language* l = Find(chosen); l && chosen != kEnglish && HasText(*l))
        {
            fprintf(stderr, "[language] %s (the game's id %d; the console's choice was %d)\n", l->name, l->id, ctx.r3.s32);
            ctx.r3.s64 = l->id;
        }
    }
    __imp__sub_824379D8(ctx, base);
}

PPC_FUNC(sub_8234D918)
{
    const uint32_t lr = uint32_t(ctx.lr);
    if (lr >= kPsSpeechPickBegin && lr < kPsSpeechPickEnd && ctx.r4.u32 == kPsSpeechCollection && ctx.r5.u32 != 0)
    {
        const auto& banks = SpeechBanks();
        if (banks[0] && (ctx.r5.u32 >= banks.size() || !banks[ctx.r5.u32]))
            ctx.r5.u64 = 0;  // the chosen language's bank isn't installed: English's
    }
    __imp__sub_8234D918(ctx, base);
}
