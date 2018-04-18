
#include <sstream>
#include <regex>

#include "mesh_loader.h"

bool LoadOBJ(fs::path const &path, std::vector<vec3> &positions, std::vector<vec3> &normals, std::vector<vec2> &uvs, std::vector<std::vector<std::size_t>> &,
             std::vector<index_t> &indices)
{
    std::ifstream file(path.native(), std::ios::in);

    if (!file.is_open()) {
        std::cerr << "can't open file: "s << path << std::endl;
        return false;
    }

    std::vector<std::size_t> face;

    //std::array<char, 256> line;
    std::string line;
    std::string attribute;

    float x, y, z;

    std::size_t inx;
    std::vector<std::size_t> inxs;
    // std::vector<index_t> indices;

    std::regex const regex_pattern("^[ |\t]*f[ |\t]+(.*)[ |\t]+(.*)[ |\t]+(.*)"s, std::regex::optimize);

    while (std::getline(file, line)) {
        std::istringstream stream(std::data(line));

        stream >> attribute;

        if (attribute == "v"s) {
            stream >> x >> y >> z;
            positions.emplace_back(x, y, z);
        }

        else if (attribute == "vn"s) {
            stream >> x >> y >> z;
            normals.emplace_back(x, y, z);
        }

        else if (attribute == "vt"s) {
            stream >> x >> y;
            uvs.emplace_back(x, y);
        }

        else if (attribute == "f"s) {
            face.clear();

            std::smatch matches;
            if (std::regex_search(line, matches, regex_pattern)) {
                if (matches.size() != 3 + 1)
                    continue;

                // indices.clear();

                for (auto index : { 1u, 2u, 3u }) {
                    std::istringstream in(matches.str(index));

                    inxs.clear();

                    bool b = true;

                    while (b) {
                        in >> inx;

                        if (in.eof() || in.bad())
                            b = false;

                        else if (in.fail()) {
                            in.clear();
                            in.ignore(std::numeric_limits<std::streamsize>::max(), '/');
                            continue;
                        }

                        inxs.emplace_back(inx - 1);
                    }

                    indices.emplace_back(index_t{ inxs.at(0), inxs.at(2), inxs.at(1) });
                }

                // faces.emplace_back(face_t{ indices.at(0), indices.at(1), indices.at(2) });
            }

            /*std::istringstream in(line.substr(1));

            bool b = true;

            while (b) {
                in >> inx;

                if (in.eof() || in.bad())
                    b = false;

                else if (in.fail()) {
                    in.clear();
                    in.ignore(std::numeric_limits<std::streamsize>::max(), '/');
                    continue;
                }

                face.emplace_back(inx);
            }

            faces.push_back(face);*/
        }
    }

    file.close();

    return true;
}