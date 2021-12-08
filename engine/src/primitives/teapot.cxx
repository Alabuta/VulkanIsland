#if 0
#include <array>
#include <tuple>
#include <ranges>
#include <variant>
#include <functional>

#include <string>
using namespace std::string_literals;

#include "utility/mpl.hxx"
#include "utility/helpers.hxx"
#include "utility/exceptions.hxx"

#include "math/math.hxx"
#include "math/pack-unpack.hxx"

#include "graphics/graphics.hxx"

#include "primitives/primitives.hxx"

/*
 * @author Eric Haines / http://erichaines.com/
 *
 * Tessellates the famous Utah teapot database by Martin Newell into triangles.
 *
 * Defaults: size = 50, segments = 10
 *
 * size is a relative scale: I've scaled the teapot to fit vertically between -1 and 1.
 * Think of it as a "radius".
 * segments - number of line segments to subdivide each patch edge;
 *   1 is possible but gives degenerates, so two is the real minimum.
 *
 * Segments 'n' determines the number of triangles output.
 *   Total triangles = 32*2*n*n - 8*n    [degenerates at the top and bottom cusps are deleted]
 *
 *   size_factor   # triangles
 *       1          56
 *       2         240
 *       3         552
 *       4         992
 *
 *      10        6320
 *      20       25440
 *      30       57360
 *
 * Code converted from my ancient SPD software, http://tog.acm.org/resources/SPD/
 * Created for the Udacity course "Interactive Rendering", http://bit.ly/ericity
 * Lesson: https://www.udacity.com/course/viewer#!/c-cs291/l-68866048/m-106482448
 * YouTube video on teapot history: https://www.youtube.com/watch?v=DxMfblPzFNc
 *
 * See https://en.wikipedia.org/wiki/Utah_teapot for the history of the teapot
 */


namespace {
    auto constexpr teapot_patches = std::array{
        /*rim*/
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        3, 16, 17, 18, 7, 19, 20, 21, 11, 22, 23, 24, 15, 25, 26, 27,
        18, 28, 29, 30, 21, 31, 32, 33, 24, 34, 35, 36, 27, 37, 38, 39,
        30, 40, 41, 0, 33, 42, 43, 4, 36, 44, 45, 8, 39, 46, 47, 12,
        /*body*/
        12, 13, 14, 15, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
        15, 25, 26, 27, 51, 60, 61, 62, 55, 63, 64, 65, 59, 66, 67, 68,
        27, 37, 38, 39, 62, 69, 70, 71, 65, 72, 73, 74, 68, 75, 76, 77,
        39, 46, 47, 12, 71, 78, 79, 48, 74, 80, 81, 52, 77, 82, 83, 56,
        56, 57, 58, 59, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
        59, 66, 67, 68, 87, 96, 97, 98, 91, 99, 100, 101, 95, 102, 103, 104,
        68, 75, 76, 77, 98, 105, 106, 107, 101, 108, 109, 110, 104, 111, 112, 113,
        77, 82, 83, 56, 107, 114, 115, 84, 110, 116, 117, 88, 113, 118, 119, 92,
        /*handle*/
        120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
        123, 136, 137, 120, 127, 138, 139, 124, 131, 140, 141, 128, 135, 142, 143, 132,
        132, 133, 134, 135, 144, 145, 146, 147, 148, 149, 150, 151, 68, 152, 153, 154,
        135, 142, 143, 132, 147, 155, 156, 144, 151, 157, 158, 148, 154, 159, 160, 68,
        /*spout*/
        161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
        164, 177, 178, 161, 168, 179, 180, 165, 172, 181, 182, 169, 176, 183, 184, 173,
        173, 174, 175, 176, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196,
        176, 183, 184, 173, 188, 197, 198, 185, 192, 199, 200, 189, 196, 201, 202, 193,
        /*lid*/
        203, 203, 203, 203, 204, 205, 206, 207, 208, 208, 208, 208, 209, 210, 211, 212,
        203, 203, 203, 203, 207, 213, 214, 215, 208, 208, 208, 208, 212, 216, 217, 218,
        203, 203, 203, 203, 215, 219, 220, 221, 208, 208, 208, 208, 218, 222, 223, 224,
        203, 203, 203, 203, 221, 225, 226, 204, 208, 208, 208, 208, 224, 227, 228, 209,
        209, 210, 211, 212, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
        212, 216, 217, 218, 232, 241, 242, 243, 236, 244, 245, 246, 240, 247, 248, 249,
        218, 222, 223, 224, 243, 250, 251, 252, 246, 253, 254, 255, 249, 256, 257, 258,
        224, 227, 228, 209, 252, 259, 260, 229, 255, 261, 262, 233, 258, 263, 264, 237,
        /*bottom*/
        265, 265, 265, 265, 266, 267, 268, 269, 270, 271, 272, 273, 92, 119, 118, 113,
        265, 265, 265, 265, 269, 274, 275, 276, 273, 277, 278, 279, 113, 112, 111, 104,
        265, 265, 265, 265, 276, 280, 281, 282, 279, 283, 284, 285, 104, 103, 102, 95,
        265, 265, 265, 265, 282, 286, 287, 266, 285, 288, 289, 270, 95, 94, 93, 92
    };

    auto constexpr teapot_vertices = std::array{
        glm::vec3{1.4f, 0.f, 2.4f},
        glm::vec3{1.4f, -0.784f, 2.4f},
        glm::vec3{0.784f, -1.4f, 2.4f},
        glm::vec3{0.f, -1.4f, 2.4f},
        glm::vec3{1.3375f, 0.f, 2.53125f},
        glm::vec3{1.3375f, -0.749f, 2.53125f},
        glm::vec3{0.749f, -1.3375f, 2.53125f},
        glm::vec3{0.f, -1.3375f, 2.53125f},
        glm::vec3{1.4375f, 0.f, 2.53125f},
        glm::vec3{1.4375f, -0.805f, 2.53125f},
        glm::vec3{0.805f, -1.4375f, 2.53125f},
        glm::vec3{0.f, -1.4375f, 2.53125f},
        glm::vec3{1.5f, 0.f, 2.4f},
        glm::vec3{1.5f, -0.84f, 2.4f},
        glm::vec3{0.84f, -1.5f, 2.4f},
        glm::vec3{0.f, -1.5f, 2.4f},
        glm::vec3{-0.784f, -1.4f, 2.4f},
        glm::vec3{-1.4f, -0.784f, 2.4f},
        glm::vec3{-1.4f, 0.f, 2.4f},
        glm::vec3{-0.749f, -1.3375f, 2.53125f},
        glm::vec3{-1.3375f, -0.749f, 2.53125f},
        glm::vec3{-1.3375f, 0.f, 2.53125f},
        glm::vec3{-0.805f, -1.4375f, 2.53125f},
        glm::vec3{-1.4375f, -0.805f, 2.53125f},
        glm::vec3{-1.4375f, 0.f, 2.53125f},
        glm::vec3{-0.84f, -1.5f, 2.4f},
        glm::vec3{-1.5f, -0.84f, 2.4f},
        glm::vec3{-1.5f, 0.f, 2.4f},
        glm::vec3{-1.4f, 0.784f, 2.4f},
        glm::vec3{-0.784f, 1.4f, 2.4f},
        glm::vec3{0.f, 1.4f, 2.4f},
        glm::vec3{-1.3375f, 0.749f, 2.53125f},
        glm::vec3{-0.749f, 1.3375f, 2.53125f},
        glm::vec3{0.f, 1.3375f, 2.53125f},
        glm::vec3{-1.4375f, 0.805f, 2.53125f},
        glm::vec3{-0.805f, 1.4375f, 2.53125f},
        glm::vec3{0.f, 1.4375f, 2.53125f},
        glm::vec3{-1.5f, 0.84f, 2.4f},
        glm::vec3{-0.84f, 1.5f, 2.4f},
        glm::vec3{0.f, 1.5f, 2.4f},
        glm::vec3{0.784f, 1.4f, 2.4f},
        glm::vec3{1.4f, 0.784f, 2.4f},
        glm::vec3{0.749f, 1.3375f, 2.53125f},
        glm::vec3{1.3375f, 0.749f, 2.53125f},
        glm::vec3{0.805f, 1.4375f, 2.53125f},
        glm::vec3{1.4375f, 0.805f, 2.53125f},
        glm::vec3{0.84f, 1.5f, 2.4f},
        glm::vec3{1.5f, 0.84f, 2.4f},
        glm::vec3{1.75f, 0.f, 1.875f},
        glm::vec3{1.75f, -0.98f, 1.875f},
        glm::vec3{0.98f, -1.75f, 1.875f},
        glm::vec3{0.f, -1.75f, 1.875f},
        glm::vec3{2.f, 0.f, 1.35f},
        glm::vec3{2.f, -1.12f, 1.35f},
        glm::vec3{1.12f, -2.f, 1.35f},
        glm::vec3{0.f, -2.f, 1.35f},
        glm::vec3{2.f, 0.f, 0.9f},
        glm::vec3{2.f, -1.12f, 0.9f},
        glm::vec3{1.12f, -2.f, 0.9f},
        glm::vec3{0.f, -2.f, 0.9f},
        glm::vec3{-0.98f, -1.75f, 1.875f},
        glm::vec3{-1.75f, -0.98f, 1.875f},
        glm::vec3{-1.75f, 0.f, 1.875f},
        glm::vec3{-1.12f, -2.f, 1.35f},
        glm::vec3{-2.f, -1.12f, 1.35f},
        glm::vec3{-2.f, 0.f, 1.35f},
        glm::vec3{-1.12f, -2.f, 0.9f},
        glm::vec3{-2.f, -1.12f, 0.9f},
        glm::vec3{-2.f, 0.f, 0.9f},
        glm::vec3{-1.75f, 0.98f, 1.875f},
        glm::vec3{-0.98f, 1.75f, 1.875f},
        glm::vec3{0.f, 1.75f, 1.875f},
        glm::vec3{-2.f, 1.12f, 1.35f},
        glm::vec3{-1.12f, 2.f, 1.35f},
        glm::vec3{0.f, 2.f, 1.35f},
        glm::vec3{-2.f, 1.12f, 0.9f},
        glm::vec3{-1.12f, 2.f, 0.9f},
        glm::vec3{0.f, 2.f, 0.9f},
        glm::vec3{0.98f, 1.75f, 1.875f},
        glm::vec3{1.75f, 0.98f, 1.875f},
        glm::vec3{1.12f, 2.f, 1.35f},
        glm::vec3{2.f, 1.12f, 1.35f},
        glm::vec3{1.12f, 2.f, 0.9f},
        glm::vec3{2.f, 1.12f, 0.9f},
        glm::vec3{2.f, 0.f, 0.45f},
        glm::vec3{2.f, -1.12f, 0.45f},
        glm::vec3{1.12f, -2.f, 0.45f},
        glm::vec3{0.f, -2.f, 0.45f},
        glm::vec3{1.5f, 0.f, 0.225f},
        glm::vec3{1.5f, -0.84f, 0.225f},
        glm::vec3{0.84f, -1.5f, 0.225f},
        glm::vec3{0.f, -1.5f, 0.225f},
        glm::vec3{1.5f, 0.f, 0.15f},
        glm::vec3{1.5f, -0.84f, 0.15f},
        glm::vec3{0.84f, -1.5f, 0.15f},
        glm::vec3{0.f, -1.5f, 0.15f},
        glm::vec3{-1.12f, -2.f, 0.45f},
        glm::vec3{-2.f, -1.12f, 0.45f},
        glm::vec3{-2.f, 0.f, 0.45f},
        glm::vec3{-0.84f, -1.5f, 0.225f},
        glm::vec3{-1.5f, -0.84f, 0.225f},
        glm::vec3{-1.5f, 0.f, 0.225f},
        glm::vec3{-0.84f, -1.5f, 0.15f},
        glm::vec3{-1.5f, -0.84f, 0.15f},
        glm::vec3{-1.5f, 0.f, 0.15f},
        glm::vec3{-2.f, 1.12f, 0.45f},
        glm::vec3{-1.12f, 2.f, 0.45f},
        glm::vec3{0.f, 2.f, 0.45f},
        glm::vec3{-1.5f, 0.84f, 0.225f},
        glm::vec3{-0.84f, 1.5f, 0.225f},
        glm::vec3{0.f, 1.5f, 0.225f},
        glm::vec3{-1.5f, 0.84f, 0.15f},
        glm::vec3{-0.84f, 1.5f, 0.15f},
        glm::vec3{0.f, 1.5f, 0.15f},
        glm::vec3{1.12f, 2.f, 0.45f},
        glm::vec3{2.f, 1.12f, 0.45f},
        glm::vec3{0.84f, 1.5f, 0.225f},
        glm::vec3{1.5f, 0.84f, 0.225f},
        glm::vec3{0.84f, 1.5f, 0.15f},
        glm::vec3{1.5f, 0.84f, 0.15f},
        glm::vec3{-1.6f, 0.f, 2.025f},
        glm::vec3{-1.6f, -0.3f, 2.025f},
        glm::vec3{-1.5f, -0.3f, 2.25f},
        glm::vec3{-1.5f, 0.f, 2.25f},
        glm::vec3{-2.3f, 0.f, 2.025f},
        glm::vec3{-2.3f, -0.3f, 2.025f},
        glm::vec3{-2.5f, -0.3f, 2.25f},
        glm::vec3{-2.5f, 0.f, 2.25f},
        glm::vec3{-2.7f, 0.f, 2.025f},
        glm::vec3{-2.7f, -0.3f, 2.025f},
        glm::vec3{-3.f, -0.3f, 2.25f},
        glm::vec3{-3.f, 0.f, 2.25f},
        glm::vec3{-2.7f, 0.f, 1.8f},
        glm::vec3{-2.7f, -0.3f, 1.8f},
        glm::vec3{-3.f, -0.3f, 1.8f},
        glm::vec3{-3.f, 0.f, 1.8f},
        glm::vec3{-1.5f, 0.3f, 2.25f},
        glm::vec3{-1.6f, 0.3f, 2.025f},
        glm::vec3{-2.5f, 0.3f, 2.25f},
        glm::vec3{-2.3f, 0.3f, 2.025f},
        glm::vec3{-3.f, 0.3f, 2.25f},
        glm::vec3{-2.7f, 0.3f, 2.025f},
        glm::vec3{-3.f, 0.3f, 1.8f},
        glm::vec3{-2.7f, 0.3f, 1.8f},
        glm::vec3{-2.7f, 0.f, 1.575f},
        glm::vec3{-2.7f, -0.3f, 1.575f},
        glm::vec3{-3.f, -0.3f, 1.35f},
        glm::vec3{-3.f, 0.f, 1.35f},
        glm::vec3{-2.5f, 0.f, 1.125f},
        glm::vec3{-2.5f, -0.3f, 1.125f},
        glm::vec3{-2.65f, -0.3f, 0.9375f},
        glm::vec3{-2.65f, 0.f, 0.9375f},
        glm::vec3{-2.f, -0.3f, 0.9f},
        glm::vec3{-1.9f, -0.3f, 0.6f},
        glm::vec3{-1.9f, 0.f, 0.6f},
        glm::vec3{-3.f, 0.3f, 1.35f},
        glm::vec3{-2.7f, 0.3f, 1.575f},
        glm::vec3{-2.65f, 0.3f, 0.9375f},
        glm::vec3{-2.5f, 0.3f, 1.125f},
        glm::vec3{-1.9f, 0.3f, 0.6f},
        glm::vec3{-2.f, 0.3f, 0.9f},
        glm::vec3{1.7f, 0.f, 1.425f},
        glm::vec3{1.7f, -0.66f, 1.425f},
        glm::vec3{1.7f, -0.66f, 0.6f},
        glm::vec3{1.7f, 0.f, 0.6f},
        glm::vec3{2.6f, 0.f, 1.425f},
        glm::vec3{2.6f, -0.66f, 1.425f},
        glm::vec3{3.1f, -0.66f, 0.825f},
        glm::vec3{3.1f, 0.f, 0.825f},
        glm::vec3{2.3f, 0.f, 2.1f},
        glm::vec3{2.3f, -0.25f, 2.1f},
        glm::vec3{2.4f, -0.25f, 2.025f},
        glm::vec3{2.4f, 0.f, 2.025f},
        glm::vec3{2.7f, 0.f, 2.4f},
        glm::vec3{2.7f, -0.25f, 2.4f},
        glm::vec3{3.3f, -0.25f, 2.4f},
        glm::vec3{3.3f, 0.f, 2.4f},
        glm::vec3{1.7f, 0.66f, 0.6f},
        glm::vec3{1.7f, 0.66f, 1.425f},
        glm::vec3{3.1f, 0.66f, 0.825f},
        glm::vec3{2.6f, 0.66f, 1.425f},
        glm::vec3{2.4f, 0.25f, 2.025f},
        glm::vec3{2.3f, 0.25f, 2.1f},
        glm::vec3{3.3f, 0.25f, 2.4f},
        glm::vec3{2.7f, 0.25f, 2.4f},
        glm::vec3{2.8f, 0.f, 2.475f},
        glm::vec3{2.8f, -0.25f, 2.475f},
        glm::vec3{3.525f, -0.25f, 2.49375f},
        glm::vec3{3.525f, 0.f, 2.49375f},
        glm::vec3{2.9f, 0.f, 2.475f},
        glm::vec3{2.9f, -0.15f, 2.475f},
        glm::vec3{3.45f, -0.15f, 2.5125f},
        glm::vec3{3.45f, 0.f, 2.5125f},
        glm::vec3{2.8f, 0.f, 2.4f},
        glm::vec3{2.8f, -0.15f, 2.4f},
        glm::vec3{3.2f, -0.15f, 2.4f},
        glm::vec3{3.2f, 0.f, 2.4f},
        glm::vec3{3.525f, 0.25f, 2.49375f},
        glm::vec3{2.8f, 0.25f, 2.475f},
        glm::vec3{3.45f, 0.15f, 2.5125f},
        glm::vec3{2.9f, 0.15f, 2.475f},
        glm::vec3{3.2f, 0.15f, 2.4f},
        glm::vec3{2.8f, 0.15f, 2.4f},
        glm::vec3{0.f, 0.f, 3.15f},
        glm::vec3{0.8f, 0.f, 3.15f},
        glm::vec3{0.8f, -0.45f, 3.15f},
        glm::vec3{0.45f, -0.8f, 3.15f},
        glm::vec3{0.f, -0.8f, 3.15f},
        glm::vec3{0.f, 0.f, 2.85f},
        glm::vec3{0.2f, 0.f, 2.7f},
        glm::vec3{0.2f, -0.112f, 2.7f},
        glm::vec3{0.112f, -0.2f, 2.7f},
        glm::vec3{0.f, -0.2f, 2.7f},
        glm::vec3{-0.45f, -0.8f, 3.15f},
        glm::vec3{-0.8f, -0.45f, 3.15f},
        glm::vec3{-0.8f, 0.f, 3.15f},
        glm::vec3{-0.112f, -0.2f, 2.7f},
        glm::vec3{-0.2f, -0.112f, 2.7f},
        glm::vec3{-0.2f, 0.f, 2.7f},
        glm::vec3{-0.8f, 0.45f, 3.15f},
        glm::vec3{-0.45f, 0.8f, 3.15f},
        glm::vec3{0.f, 0.8f, 3.15f},
        glm::vec3{-0.2f, 0.112f, 2.7f},
        glm::vec3{-0.112f, 0.2f, 2.7f},
        glm::vec3{0.f, 0.2f, 2.7f},
        glm::vec3{0.45f, 0.8f, 3.15f},
        glm::vec3{0.8f, 0.45f, 3.15f},
        glm::vec3{0.112f, 0.2f, 2.7f},
        glm::vec3{0.2f, 0.112f, 2.7f},
        glm::vec3{0.4f, 0.f, 2.55f},
        glm::vec3{0.4f, -0.224f, 2.55f},
        glm::vec3{0.224f, -0.4f, 2.55f},
        glm::vec3{0.f, -0.4f, 2.55f},
        glm::vec3{1.3f, 0.f, 2.55f},
        glm::vec3{1.3f, -0.728f, 2.55f},
        glm::vec3{0.728f, -1.3f, 2.55f},
        glm::vec3{0.f, -1.3f, 2.55f},
        glm::vec3{1.3f, 0.f, 2.4f},
        glm::vec3{1.3f, -0.728f, 2.4f},
        glm::vec3{0.728f, -1.3f, 2.4f},
        glm::vec3{0.f, -1.3f, 2.4f},
        glm::vec3{-0.224f, -0.4f, 2.55f},
        glm::vec3{-0.4f, -0.224f, 2.55f},
        glm::vec3{-0.4f, 0.f, 2.55f},
        glm::vec3{-0.728f, -1.3f, 2.55f},
        glm::vec3{-1.3f, -0.728f, 2.55f},
        glm::vec3{-1.3f, 0.f, 2.55f},
        glm::vec3{-0.728f, -1.3f, 2.4f},
        glm::vec3{-1.3f, -0.728f, 2.4f},
        glm::vec3{-1.3f, 0.f, 2.4f},
        glm::vec3{-0.4f, 0.224f, 2.55f},
        glm::vec3{-0.224f, 0.4f, 2.55f},
        glm::vec3{0.f, 0.4f, 2.55f},
        glm::vec3{-1.3f, 0.728f, 2.55f},
        glm::vec3{-0.728f, 1.3f, 2.55f},
        glm::vec3{0.f, 1.3f, 2.55f},
        glm::vec3{-1.3f, 0.728f, 2.4f},
        glm::vec3{-0.728f, 1.3f, 2.4f},
        glm::vec3{0.f, 1.3f, 2.4f},
        glm::vec3{0.224f, 0.4f, 2.55f},
        glm::vec3{0.4f, 0.224f, 2.55f},
        glm::vec3{0.728f, 1.3f, 2.55f},
        glm::vec3{1.3f, 0.728f, 2.55f},
        glm::vec3{0.728f, 1.3f, 2.4f},
        glm::vec3{1.3f, 0.728f, 2.4f},
        glm::vec3{0.f, 0.f, 0.f},
        glm::vec3{1.425f, 0.f, 0.f},
        glm::vec3{1.425f, 0.798f, 0.f},
        glm::vec3{0.798f, 1.425f, 0.f},
        glm::vec3{0.f, 1.425f, 0.f},
        glm::vec3{1.5f, 0.f, 0.075f},
        glm::vec3{1.5f, 0.84f, 0.075f},
        glm::vec3{0.84f, 1.5f, 0.075f},
        glm::vec3{0.f, 1.5f, 0.075f},
        glm::vec3{-0.798f, 1.425f, 0.f},
        glm::vec3{-1.425f, 0.798f, 0.f},
        glm::vec3{-1.425f, 0.f, 0.f},
        glm::vec3{-0.84f, 1.5f, 0.075f},
        glm::vec3{-1.5f, 0.84f, 0.075f},
        glm::vec3{-1.5f, 0.f, 0.075f},
        glm::vec3{-1.425f, -0.798f, 0.f},
        glm::vec3{-0.798f, -1.425f, 0.f},
        glm::vec3{0.f, -1.425f, 0.f},
        glm::vec3{-1.5f, -0.84f, 0.075f},
        glm::vec3{-0.84f, -1.5f, 0.075f},
        glm::vec3{0.f, -1.5f, 0.075f},
        glm::vec3{0.798f, -1.425f, 0.f},
        glm::vec3{1.425f, -0.798f, 0.f},
        glm::vec3{0.84f, -1.5f, 0.075f},
        glm::vec3{1.5f, -0.84f, 0.075f}
    };

    auto constexpr bezier_quadratic = glm::mat3{
        1, -2, 1,
        -2, 2, 0,
        1, 0, 0
    };

    auto constexpr bezier_cubic = glm::mat4{
        -1, 3, -3, 1,
        3, -6, 3, 0,
        -3, 3, 0, 0,
        1, 0, 0, 0
    };
}


namespace primitives
{
    std::uint32_t calculate_teapot_triangles_count(primitives::teapot_create_info const &create_info)
    {
        auto segments = std::max(2u, create_info.segments);

        auto triangles_count = static_cast<std::uint32_t>(create_info.bottom) * (8 * segments - 4) * segments;
        triangles_count += static_cast<std::uint32_t>(create_info.lid) * (16 * segments - 4) * segments;
        triangles_count += static_cast<std::uint32_t>(create_info.body) * 40 * segments * segments;

        return triangles_count;
    }

    std::uint32_t calculate_teapot_vertices_count(primitives::teapot_create_info const &create_info)
    {
        auto segments = std::max(2u, create_info.segments);

        auto vertices_count = static_cast<std::uint32_t>(create_info.bottom) * 4;
        vertices_count += static_cast<std::uint32_t>(create_info.lid) * 8;
        vertices_count += static_cast<std::uint32_t>(create_info.body) * 20;
        vertices_count *= (segments + 1) * (segments + 1);

        return vertices_count;
    }

    std::uint32_t calculate_teapot_indices_count(primitives::teapot_create_info const &create_info)
    {
        return calculate_teapot_triangles_count(create_info) * 3;
    }

    void g(primitives::teapot_create_info const &create_info)
    {
#if 0
        auto constexpr blinn_scale = 1.3f;

        // scale the size to be the real scaling factor
        auto max_height = 3.15f * (blinn ? 1.f : blinn_scale);

        auto max_height2 = max_height / 2;
        auto true_size = create_info.size / max_height2;

        auto segments = std::max(2u, create_info.segments);

        auto ms = glm::mat4{
            -1,  3, -3,  1,
             3, -6,  3,  0,
            -3,  3,  0,  0,
             1,  0,  0,  0
        };

        auto mgm = std::array{
            glm::mat4{1},
            glm::mat4{1},
            glm::mat4{1}
        };

        auto const [min_patches, max_patches] = std::pair(body ? 0 : 20, create_info.bottom ? 32 : 28);

        auto const vertices_per_row = segments + 1;
        auto constexpr eps = 0.00001f;

        auto surfCount = 0;

        auto vertCount = 0;
        auto normCount = 0;
        auto uvCount = 0;

        auto indexCount = 0;

        // internal function: test if triangle has any matching vertices;
        // if so, don't save triangle, since it won't display anything.
        auto notDegenerate = [] (auto vtx1, auto vtx2, auto vtx3)
        {
            // if any vertex matches, return false
            return !(((vertices[vtx1 * 3] == = vertices[vtx2 * 3]) &&
                (vertices[vtx1 * 3 + 1] == = vertices[vtx2 * 3 + 1]) &&
                (vertices[vtx1 * 3 + 2] == = vertices[vtx2 * 3 + 2])) ||

                ((vertices[vtx1 * 3] == = vertices[vtx3 * 3]) &&
                (vertices[vtx1 * 3 + 1] == = vertices[vtx3 * 3 + 1]) &&
                (vertices[vtx1 * 3 + 2] == = vertices[vtx3 * 3 + 2])) ||

                ((vertices[vtx2 * 3] == = vertices[vtx3 * 3]) &&
                (vertices[vtx2 * 3 + 1] == = vertices[vtx3 * 3 + 1]) &&
                (vertices[vtx2 * 3 + 2] == = vertices[vtx3 * 3 + 2])));
        };

        for (auto surf = minPatches; surf < maxPatches; ++surf) {
            // lid is in the middle of the data, patches 20-27,
            // so ignore it for this part of the loop if the lid is not desired
            if (lid || (surf < 20 || surf >= 28)) {
                // get M * G * M matrix for x,y,z
                for (i = 0; i < 3; i++) {
                    // get control patches
                    for (r = 0; r < 4; r++) {
                        for (c = 0; c < 4; c++) {
                            // transposed
                            g[c * 4 + r] = teapot_vertices[teapot_patches[surf * 16 + r * 4 + c] * 3 + i];

                            // is the lid to be made larger, and is this a point on the lid
                            // that is X or Y?
                            if (fitLid && (surf >= 20 && surf < 28) && (i != = 2)) {
                                // increase XY size by 7.7%, found empirically. I don't
                                // increase Z so that the teapot will continue to fit in the
                                // space -1 to 1 for Y (Y is up for the final model).
                                g[c * 4 + r] *= 1.077;
                            }

                            // Blinn "fixed" the teapot by dividing Z by blinnScale, and that's the
                            // data we now use. The original teapot is taller. Fix it:
                            if (!blinn && (i == 2))
                                g[c * 4 + r] *= blinnScale;
                        }
                    }

                    gmx.set(g[0], g[1], g[2], g[3], g[4], g[5], g[6], g[7], g[8], g[9], g[10], g[11], g[12], g[13], g[14], g[15]);

                    tmtx.multiplyMatrices(gmx, ms);
                    mgm[i].multiplyMatrices(mst, tmtx);
                }

                // step along, get points, and output
                for (sstep = 0; sstep <= segments; sstep++) {
                    s = sstep / segments;
                    for (tstep = 0; tstep <= segments; tstep++) {
                        t = tstep / segments;

                        // point from basis
                        // get power vectors and their derivatives
                        for (p = 4, sval = tval = 1.0; p--;) {
                            sp[p] = sval;
                            tp[p] = tval;
                            sval *= s;
                            tval *= t;

                            if (p == = 3) {
                                dsp[p] = dtp[p] = 0.0;
                                dsval = dtval = 1.0;
                            }

                            else {
                                dsp[p] = dsval * (3 - p);
                                dtp[p] = dtval * (3 - p);
                                dsval *= s;
                                dtval *= t;
                            }
                        }

                        vsp.fromArray(sp);
                        vtp.fromArray(tp);
                        vdsp.fromArray(dsp);
                        vdtp.fromArray(dtp);

                        // do for x,y,z
                        for (i = 0; i < 3; i++) {
                            // multiply power vectors times matrix to get value
                            tcoord = vsp.clone();
                            tcoord.applyMatrix4(mgm[i]);
                            vert[i] = tcoord.dot(vtp);

                            // get s and t tangent vectors
                            tcoord = vdsp.clone();
                            tcoord.applyMatrix4(mgm[i]);
                            sdir[i] = tcoord.dot(vtp);

                            tcoord = vsp.clone();
                            tcoord.applyMatrix4(mgm[i]);
                            tdir[i] = tcoord.dot(vdtp);
                        }

                        // find normal
                        vsdir.fromArray(sdir);
                        vtdir.fromArray(tdir);
                        norm.crossVectors(vtdir, vsdir);
                        norm.normalize();

                        // if X and Z length is 0, at the cusp, so point the normal up or down, depending on patch number
                        if (vert[0] == = 0 && vert[1] == = 0)
                            // if above the middle of the teapot, normal points up, else down
                            normOut.set(0, vert[2] > maxHeight2 ? 1 : -1, 0);
                        else
                            // standard output: rotate on X axis
                            normOut.set(norm.x, norm.z, -norm.y);

                        // store it all
                        vertices[vertCount++] = trueSize * vert[0];
                        vertices[vertCount++] = trueSize * (vert[2] - maxHeight2);
                        vertices[vertCount++] = -trueSize * vert[1];

                        normals[normCount++] = normOut.x;
                        normals[normCount++] = normOut.y;
                        normals[normCount++] = normOut.z;

                        uvs[uvCount++] = 1 - t;
                        uvs[uvCount++] = 1 - s;
                    }
                }

                // save the faces
                for (sstep = 0; sstep < segments; sstep++) {
                    for (tstep = 0; tstep < segments; tstep++) {
                        v1 = surfCount * vertPerRow * vertPerRow + sstep * vertPerRow + tstep;
                        v2 = v1 + 1;
                        v3 = v2 + vertPerRow;
                        v4 = v1 + vertPerRow;

                        // Normals and UVs cannot be shared. Without clone(), you can see the consequences
                        // of sharing if you call geometry.applyMatrix( matrix ).
                        if (notDegenerate(v1, v2, v3)) {
                            indices[indexCount++] = v1;
                            indices[indexCount++] = v2;
                            indices[indexCount++] = v3;
                        }

                        if (notDegenerate(v1, v3, v4)) {
                            indices[indexCount++] = v1;
                            indices[indexCount++] = v3;
                            indices[indexCount++] = v4;
                        }
                    }
                }

                // increment only if a surface was used
                surfCount++;
            }
        }
#endif
    }

    void generate_teapot_indexed(primitives::teapot_create_info const &create_info, std::span<std::byte> vertex_buffer,
                                 std::span<std::byte> index_buffer)
    {
        auto indices_number = calculate_teapot_indices_count(create_info);

        switch (create_info.index_buffer_type) {
            case graphics::INDEX_TYPE::UINT_16:
            {
                using pointer_type = typename std::add_pointer_t<std::uint16_t>;
                auto it = reinterpret_cast<pointer_type>(std::to_address(std::data(index_buffer)));

                //generate_indices(create_info, it, indices_number);
            }
            break;

            case graphics::INDEX_TYPE::UINT_32:
            {
                using pointer_type = typename std::add_pointer_t<std::uint32_t>;
                auto it = reinterpret_cast<pointer_type>(std::to_address(std::data(index_buffer)));

                //generate_indices(create_info, it, indices_number);
            }
            break;

            default:
                throw resource::exception("unsupported index instance type"s);
        }

        generate_teapot(create_info, vertex_buffer);
    }

    void generate_teapot(primitives::teapot_create_info const &create_info, std::span<std::byte> vertex_buffer)
    {
        auto &&vertex_layout = create_info.vertex_layout;

        auto vertex_number = calculate_teapot_vertices_count(create_info);
        auto vertex_size = static_cast<std::uint32_t>(vertex_layout.size_bytes);

        auto &&attributes = vertex_layout.attributes;

        for (std::size_t offset_in_bytes = 0; auto && attribute : attributes) {
            if (auto format_inst = graphics::instantiate_format(attribute.format); format_inst) {
                std::visit([&] <typename T> (T &&)
                {
                    using type = typename std::remove_cvref_t<T>;
                    using pointer_type = typename std::add_pointer_t<type>;

                    auto data = reinterpret_cast<pointer_type>(std::to_address(std::data(vertex_buffer)) + offset_in_bytes);

                    offset_in_bytes += sizeof(type);

                    auto it = strided_bidirectional_iterator<type>{data, vertex_size};

                    switch (attribute.semantic) {
                        case vertex::SEMANTIC::POSITION:
                            //generate_positions(create_info, attribute.format, std::span{transforms}, it, vertex_number);
                            break;

                        case vertex::SEMANTIC::NORMAL:
                            //generate_normals(create_info, attribute.format, std::span{transforms}, it, vertex_number);
                            break;

                        case vertex::SEMANTIC::TEXCOORD_0:
                            //generate_texcoords(create_info, attribute.format, it, vertex_number);
                            break;

                        case vertex::SEMANTIC::COLOR_0:
                            //generate_colors(create_info, attribute.format, it, vertex_number);
                            break;

                        default:
                            break;
                    }

                }, *format_inst);
            }

            else throw resource::exception("unsupported attribute format"s);
        }
    }
}
#endif