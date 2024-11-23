//
// Created by engin on 22/09/2024.
//

#ifndef LIMONENGINE_VISIBILITYREQUEST_H
#define LIMONENGINE_VISIBILITYREQUEST_H


class VisibilityRequest {
public:
        class uint64_vector_hasher {
        public:
            std::size_t operator()(std::vector<uint64_t> const &vec) const {
                std::size_t seed = vec.size();
                for (auto x: vec) {
                    x = ((x >> 16) ^ x) * 0x45d9f3b;
                    x = ((x >> 16) ^ x) * 0x45d9f3b;
                    x = (x >> 16) ^ x;
                    seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                }
                return seed;
            }
        };

        static SDL2MultiThreading::Condition condition;
        const Camera* const camera;
        glm::vec3 playerPosition;
        const OptionsUtil::Options* options;
        const OptionsUtil::Options::Option<std::vector<long>> lodDistancesOption;
        const OptionsUtil::Options::Option<double> skipRenderDistanceOption;
        const OptionsUtil::Options::Option<double> skipRenderSizeOption;
        const OptionsUtil::Options::Option<double> maxSkipRenderSizeOption;
        const std::unordered_map<uint32_t, PhysicalRenderable *>* const objects;
        std::unordered_map<std::vector<uint64_t>, std::unordered_map<uint32_t , std::pair<std::vector<uint32_t>, uint32_t>>, uint64_vector_hasher> * visibility;
        bool running = true;
        std::atomic<uint32_t> frameCount;
        SDL2MultiThreading::SpinLock inProgressLock;
        SDL_mutex* blockMutex = SDL_CreateMutex();
        VisibilityRequest(Camera* camera, std::unordered_map<uint32_t, PhysicalRenderable *>* objects, std::unordered_map<std::vector<uint64_t>, std::unordered_map<uint32_t , std::pair<std::vector<uint32_t>, uint32_t>>, uint64_vector_hasher> * visibility, const glm::vec3& playerPosition, const OptionsUtil::Options* options) :
                camera(camera), playerPosition(playerPosition), options(options),
                lodDistancesOption(options->getOption<std::vector<long>>(HASH("LodDistanceList"))),
                skipRenderDistanceOption(options->getOption<double>(HASH("SkipRenderDistance"))),
                skipRenderSizeOption(options->getOption<double>(HASH("SkipRenderSize"))),
                maxSkipRenderSizeOption(options->getOption<double>(HASH("MaxSkipRenderSize"))),
                objects(objects), visibility(visibility), frameCount(0) {

        };

        std::unordered_map<std::vector<uint64_t>, std::unordered_map<uint32_t , std::pair<std::vector<uint32_t>, uint32_t>>>::iterator findHashEntry(uint64_t hash) const {
            std::unordered_map<std::vector<uint64_t>, std::unordered_map<uint32_t , std::pair<std::vector<uint32_t>, uint32_t>>>::iterator it = visibility->begin();
            for (;
                    it != visibility->end(); it++){
                for(std::vector<uint64_t>::const_iterator hashIt = it->first.begin(); hashIt != it->first.end(); hashIt++){
                    if((*hashIt) == hash) {
                        return it;
                    }
                }
            }
            return it;//this returns the end iterator, so it needs to be checked by the caller
        }

        static std::unordered_map<std::vector<uint64_t>, std::unordered_map<uint32_t , std::pair<std::vector<uint32_t>, uint32_t>>>::iterator findHashEntry(
                std::unordered_map<std::vector<uint64_t>, std::unordered_map<uint32_t , std::pair<std::vector<uint32_t>, uint32_t>>, uint64_vector_hasher> * visibility,
                uint64_t hash)  {
            std::unordered_map<std::vector<uint64_t>, std::unordered_map<uint32_t , std::pair<std::vector<uint32_t>, uint32_t>>>::iterator it = visibility->begin();
            for (;
                    it != visibility->end(); it++){
                for(std::vector<uint64_t>::const_iterator hashIt = it->first.begin(); hashIt != it->first.end(); hashIt++){
                    if((*hashIt) == hash) {
                        return it;
                    }
                }
            }
            return it;//this returns the end iterator, so it needs to be checked by the caller
        }
};


#endif //LIMONENGINE_VISIBILITYREQUEST_H
