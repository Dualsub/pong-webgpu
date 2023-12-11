
#include "pong/Model.h"

#include <unordered_map>
#include <string>

namespace pong
{
    struct AssetPack
    {
    };

    class Resources
    {

    private:
        std::unordered_map<std::string, Model> models = {};

    public:
        void LoadAssetsFromPack(const std::string &filePath)
        {
        }
    };
}