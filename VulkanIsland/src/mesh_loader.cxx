
#include <sstream>

#include "mesh_loader.h"

bool LoadOBJ(fs::path const &path, std::vector<vec3> &positions, std::vector<vec3> &normals, std::vector<vec2> &uvs, std::vector<std::vector<std::size_t>> &faces)
{
    std::ifstream file(path.native(), std::ios::in);

    if (!file.is_open()) {
        std::cerr << "can't open file: "s << path << std::endl;
        return false;
    }

    std::vector<std::size_t> face;

    std::array<char, 256> line;
    std::string attribute;

    float x, y, z;
    std::size_t inx;

    while (file.getline(std::data(line), std::size(line))) {
        std::istringstream stream(std::data(line));

        stream >> attribute;

        if (attribute == "v") {
            stream >> x >> y >> z;
            positions.emplace_back(x, y, z);
        }

        else if (attribute == "vn") {
            stream >> x >> y >> z;
            normals.emplace_back(x, y, z);
        }

        else if (attribute == "vt") {
            stream >> x >> y;
            uvs.emplace_back(x, y);
        }

        else if (attribute == "f") {
            face.clear();

            std::istringstream in(&line[1]);

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

            faces.push_back(face);
        }
    }

    file.close();

    return true;
}