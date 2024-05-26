//
// Created by engin on 16/02/2024.
//

#ifndef STRINGUTILS_HPP
#define STRINGUTILS_HPP

#include <iterator>
#include <string>
#include <vector>
#include <sstream>

class StringUtils {
public:
    std::vector<std::string> static split(const std::string& source, const std::string& delimiter)
    {
        std::vector<std::string> resultVector;
        if (!source.empty()) {
            std::string::size_type start = 0;
            do {
                size_t delimiterLocation = source.find(delimiter, start);
                if (delimiterLocation == std::string::npos)
                    break;
                //clean up empty ones
                if(delimiterLocation != start) {
                    resultVector.push_back(source.substr(start, delimiterLocation-start));
                }
                start = delimiterLocation + delimiter.size();
            }
            while (true);
            if(source.length() > start) {
                resultVector.push_back(source.substr(start));
            }
        }
        return resultVector;
    }

    std::string static join(const std::vector<std::string>& source, const std::string& delimiter)
    {
        std::ostringstream joinedStream;
        std::vector<std::string>::const_iterator b = source.begin();
        std::vector<std::string>::const_iterator e = source.end();
        if(b != e) {
            std::copy(b, std::prev(e), std::ostream_iterator<std::string>(joinedStream, delimiter.c_str()));
            b = std::prev(e);
        }
        if(b != e) {
            joinedStream << *b;
        }
        return joinedStream.str();
    }
};



#endif //STRINGUTILS_HPP
