// Link stubs for the voice-transcoding functions upstream implements in Utils_silk.cpp /
// Utils_audio.cpp, which this Linux build cannot compile (the silk SDK, lame and opencore-amrnb
// are not packaged). MessageParser.cpp and AsyncTask.cpp reference them, but MessageParser only
// reaches them (parseVoice, TaskManager::convertAudio -> Mp3Task) when ExportOption is NOT in text
// mode, and fuzz_message_parser runs in text mode. They abort() so that any path that does reach
// them shows up as a crash instead of silently returning.
#include <stdlib.h>
#include "Utils.h"

bool amrToPcm(const std::string&, std::vector<unsigned char>&, std::string*) { abort(); }
bool amrToPcm(const std::string&, const std::string&, std::string*) { abort(); }
bool silkToPcm(const std::string&, std::vector<unsigned char>&, bool&, std::string*) { abort(); }
bool silkToPcm(const std::string&, const std::string&, bool&, std::string*) { abort(); }
bool pcmToMp3(const std::string&, const std::string&, std::string*) { abort(); }
bool pcmToMp3(const std::vector<unsigned char>&, const std::string&, std::string*) { abort(); }
bool amrPcmToMp3(const std::string&, const std::string&, std::string*) { abort(); }
bool amrPcmToMp3(const std::vector<unsigned char>&, const std::string&, std::string*) { abort(); }
