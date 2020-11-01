#include <map>
#include <string>
#include <vector>

#include "serialize.h"

namespace fheroes2
{
    class AGGFile
    {
    public:
        AGGFile();

        bool isGood() const;
        bool Open( const std::string & );
        const std::vector<uint8_t> & Read( uint32_t key );
        const std::vector<uint8_t> & Read( const std::string & key );

    private:
        static uint32_t aggFilenameHash( const std::string & s );

        static const size_t maxFilenameSize = 15; // 8.3 ASCIIZ file name + 2-bytes padding

        StreamFile stream;
        std::map<uint32_t, std::pair<uint32_t, uint32_t>> files;
        std::map<std::string, uint32_t> names;

        std::string key;
        std::vector<uint8_t> body;
    };
}
