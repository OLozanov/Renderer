#include "SkyBox.h"

SkyBox::SkyBox(const std::string& name)
: m_vertices{{{1.0, 1.0, 1.0 }, {1.0, 1.0}}, {{1.0, -1.0, 1.0}, {1.0, 0.0}}, {{-1.0, 1.0, 1.0}, {0.0, 1.0}}, {{-1.0, -1.0, 1.0}, {0.0, 0.0}},
             {{1.0, -1.0, -1.0}, {0.0, 0.0}}, {{ 1.0, 1.0, -1.0 }, {0.0, 1.0}}, {{-1.0, -1.0, -1.0}, {1.0, 0.0}}, {{ -1.0, 1.0, -1.0 }, {1.0, 1.0}},
             {{-1.0,  1.0, 1.0 }, {1.0, 1.0}}, {{-1.0, -1.0, 1.0}, {1.0, 0.0}}, {{-1.0,  1.0, -1.0}, {0.0, 1.0}}, {{-1.0, -1.0, -1.0}, {0.0, 0.0}},
             {{1.0, -1.0, 1.0}, {0.0, 0.0}}, {{1.0, 1.0, 1.0 }, {0.0, 1.0}}, {{1.0, -1.0, -1.0}, {1.0, 0.0}}, {{1.0, 1.0, -1.0 }, {1.0, 1.0}},
             {{1.0, 1.0, -1.0}, {0.0, 1.0}}, {{1.0, 1.0, 1.0 }, {1.0, 1.0}}, {{-1.0, 1.0, -1.0}, {0.0, 0.0}}, {{-1.0, 1.0, 1.0 }, {1.0, 0.0}},
             {{1.0, -1.0, 1.0}, {1.0, 0.0}}, {{1.0, -1.0, -1.0}, {0.0, 0.0}}, {{-1.0, -1.0, 1.0}, {1.0, 1.0}}, {{-1.0, -1.0, -1.0}, {0.0, 1.0}}}
, m_indices{0, 1, 2, 1, 3, 2,
            4, 5, 6, 5, 7, 6,
            8, 9, 10, 9, 11, 10,
            12, 13, 14, 13, 15, 14,
            16, 17, 18, 17, 19, 18,
            20, 21, 22, 21, 23, 22 }
{
    m_vertexBuffer.data = reinterpret_cast<uint8_t*>(m_vertices);
    m_vertexBuffer.stride = sizeof(SimpleVertex);

    std::string ftimage = name + "_ft.bmp";
    std::string bkimage = name + "_bk.bmp";
    std::string rtimage = name + "_rt.bmp";
    std::string lfimage = name + "_lf.bmp";
    std::string upimage = name + "_up.bmp";
    std::string dnimage = name + "_dn.bmp";

    m_faces[0] = LoadBmp(ftimage.c_str());
    m_faces[1] = LoadBmp(bkimage.c_str());
    m_faces[2] = LoadBmp(rtimage.c_str());
    m_faces[3] = LoadBmp(lfimage.c_str());
    m_faces[4] = LoadBmp(upimage.c_str());
    m_faces[5] = LoadBmp(dnimage.c_str());
}

SkyBox::~SkyBox()
{
    for (size_t i = 0; i < 6; i++) delete m_faces[i];
}