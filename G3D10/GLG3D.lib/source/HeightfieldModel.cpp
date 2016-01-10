#include "GLG3D/HeightfieldModel.h"
#include "G3D/RayGridIterator.h"
#include "GLG3D/Shader.h"
#include "G3D/Any.h"
#include "G3D/Image.h"
#include "G3D/MeshAlg.h"

namespace G3D {

static bool isInteger(float f) {
    return f == iRound(f);
}

const String& HeightfieldModel::className() const {
    static String s("HeightfieldModel");
    return s;
}


HeightfieldModel::Specification::Specification() :
    pixelsPerTileSide(128),
    pixelsPerQuadSide(1),
    metersPerPixel(1.0f),
    metersPerTexCoord(1.0f),
    maxElevation(32.0) {}
    

HeightfieldModel::Specification::Specification(const Any& any) {
    *this = Specification();

    AnyTableReader r("HeightfieldModel::Specification", any);

    r.getFilenameIfPresent("filename", filename);

    r.getIfPresent("pixelsPerTileSide", pixelsPerTileSide),
    r.getIfPresent("metersPerPixel",    metersPerPixel);
    r.getIfPresent("maxElevation",      maxElevation);
    r.getIfPresent("metersPerTexCoord", metersPerTexCoord);
    r.getIfPresent("pixelsPerQuadSide", pixelsPerQuadSide);

    r.getIfPresent("material",          material);
}


HeightfieldModel::HeightfieldModel(const Specification& spec, const String& name) : m_specification(spec), m_name(name) {

    // Allow full Texture post-processing before reading back to the CPU as an image
    const bool generateMipMaps = false;
    m_elevation = Texture::fromFile(System::findDataFile(m_specification.filename), ImageFormat::R32F(), Texture::DIM_2D, generateMipMaps);
    m_elevationImage = m_elevation->toImage();

    const float f = float(m_specification.pixelsPerTileSide) / m_specification.pixelsPerQuadSide;
    debugAssertM(isInteger(f), "pixelsPerTileSide / quadsPerPixelSide must be an integer");
#   if defined(__GNUC__) && ! defined(G3D_DEBUG)
        // Suppress a warning on gcc
        (void)isInteger;
#   endif

    m_quadsPerTileSide = iRound(f);

    debugAssertM(isInteger(float(m_elevation->width()) / m_specification.pixelsPerTileSide) && 
        isInteger(float(m_elevation->height()) / m_specification.pixelsPerTileSide),
        "Heightfield and tile dimensions must create an integer number of square tiles.");

    m_material  = UniversalMaterial::create(m_specification.material);

    loadShaders();
    generateGeometry();
}


void HeightfieldModel::generateGeometry() {
    Array<Point3> cpuPosition3;
    Array<Point2> cpuPosition2;
    Array<int>    cpuIndex;

    // Generate into cpuPosition3...and then overwrite the "texture coordinates"
    // with the XZ components

    MeshAlg::generateGrid(cpuPosition3, cpuPosition2, cpuIndex,
        m_quadsPerTileSide, m_quadsPerTileSide,
        Vector2(1, 1), false, false, Matrix3::identity() * (float)m_quadsPerTileSide);

    for (int i = 0; i < cpuPosition3.size(); ++i) {
        cpuPosition2[i] = cpuPosition3[i].xz();
    }
    debugAssertM(cpuPosition2[0].x == 0, "Coordinates generated with the wrong scale");
    debugAssertM(cpuPosition2.last().x == m_quadsPerTileSide, "Coordinates generated with the wrong scale");

    // Upload to the GPU
    shared_ptr<VertexBuffer> vb = VertexBuffer::create(sizeof(Point2) * cpuPosition2.size() + sizeof(int) * cpuIndex.size() + 16, VertexBuffer::WRITE_ONCE);
    m_positionArray = AttributeArray(cpuPosition2, vb);
    m_indexStream   = IndexStream(cpuIndex, vb);
}


G3D_DECLARE_SYMBOL(texCoordsPerMeter);
G3D_DECLARE_SYMBOL(scale);
G3D_DECLARE_SYMBOL(position);
G3D_DECLARE_SYMBOL(elevation);
G3D_DECLARE_SYMBOL(HAS_ALPHA);
G3D_DECLARE_SYMBOL(pixelsPerQuadSide);
void HeightfieldModel::setShaderArgs(Args& args) const {
    // Heightfield args
    args.setAttributeArray(SYMBOL_position, m_positionArray);
    args.setIndexStream(m_indexStream);
    args.setUniform(SYMBOL_elevation, m_elevation, Sampler::video());

    // The position vertex array values are integers 
    const float metersPerQuad = m_specification.metersPerPixel * m_specification.pixelsPerQuadSide;

    args.setUniform(SYMBOL_pixelsPerQuadSide, float(m_specification.pixelsPerQuadSide));
    args.setUniform(SYMBOL_scale, Vector3(metersPerQuad, m_specification.maxElevation, metersPerQuad));

    // Material texture coordinates per meter
    args.setUniform(SYMBOL_texCoordsPerMeter, 1.0f / m_specification.metersPerTexCoord);
	
    m_material->setShaderArgs(args, "material.");
    const shared_ptr<Texture>& lambertian = m_material->bsdf()->lambertian().texture();
    args.setMacro("HAS_ALPHA", (notNull(lambertian) && ! lambertian->opaque()) ? 1 : 0);
}


void HeightfieldModel::loadShaders() {
    m_shader = Shader::fromFiles
        (System::findDataFile("shader/HeightfieldModel/HeightfieldModel_Tile_render.vrt"), 
         System::findDataFile("shader/HeightfieldModel/HeightfieldModel_Tile_render.pix"));

    m_gbufferShader = Shader::fromFiles
        (System::findDataFile("shader/HeightfieldModel/HeightfieldModel_Tile_gbuffer.vrt"), 
         System::findDataFile("shader/HeightfieldModel/HeightfieldModel_Tile_gbuffer.pix"));

    m_depthAndColorShader = Shader::fromFiles
        (System::findDataFile("shader/HeightfieldModel/HeightfieldModel_Tile_depth.vrt"), 
         System::findDataFile("shader/HeightfieldModel/HeightfieldModel_Tile_depth.pix"));
}


void HeightfieldModel::pose(const CFrame& frame, const CFrame& previousFrame, Array<shared_ptr<Surface> >& surfaceArray, const shared_ptr<Entity>& entity,
                            const Surface::ExpressiveLightScatteringProperties& expressiveLightScatteringProperties) const {
    // Create tiles
    const Point2int32 numTiles(m_elevation->width() / m_specification.pixelsPerTileSide, m_elevation->height() / m_specification.pixelsPerTileSide);
    for (Point2int32 t(0,0); t.y < numTiles.y; ++t.y) {
        for (t.x = 0; t.x < numTiles.x; ++t.x) {
            surfaceArray.append(shared_ptr<Surface>(new Tile(this, t, frame, previousFrame, entity, expressiveLightScatteringProperties)));
        }
    }
}
    

bool HeightfieldModel::intersect
   (const Ray&                  r, 
    const CFrame&               cframe, 
    float&                      maxDistance, 
    Model::HitInfo&             info,
    const shared_ptr<Entity>&   entity) {

    bool first  = true;
    bool top    = false;

    bool hit = false;                                 
    float w0 = 0, w1 = 0, w2 = 0;
    float w3 = 0, w4 = 0, w5 = 0;
    const Ray& intRay = cframe.toObjectSpace(r);

    const int   trisPerTile     = m_quadsPerTileSide * m_quadsPerTileSide * 2;
    const int   tilesPerWidth   = m_elevation->width() / m_specification.pixelsPerTileSide;
    const float metersPerQuad   = m_specification.metersPerPixel * m_specification.pixelsPerQuadSide;
    const int   widthQuads      = m_elevation->width() / (m_specification.pixelsPerQuadSide) - 1;
    const int   heightQuads     = m_elevation->height() / (m_specification.pixelsPerQuadSide) - 1; 

    RayGridIterator rgi(intRay, 
                        Vector3int32(widthQuads - 1, 1, heightQuads - 1),
                        Vector3(metersPerQuad, m_specification.maxElevation, metersPerQuad));
    
    const int xOffset[4] = {0, 1, 1, 0};
    const int zOffset[4] = {0, 0, 1, 1};
    const Vector3int32& step = rgi.step(); 
    float dy = intRay.direction().y;
    float y1 = intRay.origin().y + rgi.enterDistance() * dy;
    float y2;
    
    Point3 p[4];
    for(; rgi.insideGrid() || rgi.enterDistance() > maxDistance; ++rgi) {
        y2 = y1 + dy * (rgi.exitDistance() - rgi.enterDistance()); 
        const Vector3int32& index = rgi.index();

        // Must be here because the last index has coordinates out of range
        if ((index.x < 0) || (index.y < 0) || (index.z < 0) || (index.x >= widthQuads) || (index.z >= heightQuads)) {
            break;
        }
        
        for (int i = 0; i < 4; ++i) {
            p[i] = Vector3((index.x + xOffset[i]) * metersPerQuad, 0, (index.z + zOffset[i]) * metersPerQuad);
            p[i].y = m_elevationImage->nearest((index.x + xOffset[i]) * m_specification.pixelsPerQuadSide, (index.z + zOffset[i]) * m_specification.pixelsPerQuadSide).rgb().sum()/(3.0f) * m_specification.maxElevation;
        }

        // Check to see if the ray is orignally above or below the heightfield so as to elimante half of the floating point checks.
        if (first) {
            first = false;
            top = (y1 > p[0].y) && (y1 > p[1].y) && (y1 > p[2].y) && (y1 > p[3].y);
        }
        
        /**
            keep track of which direction the ray entered the cell from so that we do not have to recheck certain points that we checked in the last box
           ^
           x 
            ________ 
           |1      2|
           |     /  |
           |   /    |
           | /      |
           |0______3| z >
          
           For example, if the ray propagates +x direction, then the algorithm does not have to check the heights of points 0 and 3. 
        */

        // Check to see if the two vertices that are new are above or
        // below the intersection of the ray

        bool originalFour = false;
        switch (rgi.enterAxis()) {
        case 0:
            if (step.x == 1) {
                if (top) {              
                    originalFour = (y1 < max(p[2].y, p[1].y));
                } else {
                    originalFour = (y1 > min(p[2].y, p[1].y));
                }
            } else if (step.x == -1) {
                if (top) {
                    originalFour = (y1 < max(p[0].y, p[3].y));
                } else {
                    originalFour = (y1 > min(p[0].y, p[3].y));
                }
            }
            break;

        case 1:
            // Do nothing because this is the ray entering the grid along the y axis 
            break;

        case 2:
            if (step.z == 1) {
                if (top) {
                    originalFour = (y1 < max(p[2].y, p[3].y));
                } else {
                    originalFour = (y1 > min(p[2].y, p[3].y));
                }   
            } else if (step.z == -1) {
                if (top) {
                    originalFour = (y1 < max(p[0].y, p[1].y));
                } else {
                    originalFour = (y1 > min(p[0].y, p[1].y));
                }
            }
            break;
        }

        
        bool notCulled; 
        if (top) { 
            notCulled = originalFour || (y2 < max(p[0].y, p[1].y, p[2].y, p[3].y)); 
        } else {
            notCulled = originalFour || (y2 > min(p[0].y, p[1].y, p[2].y, p[3].y));
        }

        // Only check the triangles in that grid element if one of the
        // points of intersection of the ray on the grid is between
        // the height of two of the corners of the grid
        if (notCulled) {

            float d0 = intRay.intersectionTime(p[0], p[3], p[2], w0, w1, w2); 
            float d1 = intRay.intersectionTime(p[0], p[2], p[1], w3, w4, w5);
            
            // Ignore intersectins behind the ray origin.
            if (d0 < 0) {
                d0 = finf();
            }
            if (d1 < 0) {
                d1 = finf();
            }

            const bool hitTri0 = (d0 < d1);
            const float firstIntersectDistance = min(d0, d1);

            if (firstIntersectDistance < maxDistance) {
                maxDistance = firstIntersectDistance;

                const Vector3int32 tileIndex(index.x / m_quadsPerTileSide, 0, index.z / m_quadsPerTileSide);

                const int trisPerQuad = 2;

                const int primIndex = 
                    tileIndex.x * trisPerTile +
                    tileIndex.z * trisPerTile * tilesPerWidth +
                    (index.x - m_quadsPerTileSide * tileIndex.z) * trisPerQuad + 
                    (index.z - m_quadsPerTileSide * tileIndex.x) * m_quadsPerTileSide * trisPerQuad +
                    (hitTri0 ? 0 : 1);

                const Vector3& normal = 
                    hitTri0 ? 
                    (p[2] - p[0]).cross(p[3] - p[0]).direction() : 
                    (p[1] - p[0]).cross(p[2] - p[0]).direction();

                const Point3& intersectionPoint = r.origin() + firstIntersectDistance * r.direction();

                info.set
                    (dynamic_pointer_cast<HeightfieldModel>(shared_from_this()), 
                     entity, 
                     m_material,
                     normal,
                     intersectionPoint,
                     "",
                     0,
                     primIndex,
                     hitTri0 ? w0 : w3,
                     hitTri0 ? w2 : w5);
                hit = true;
            } // if hit before maxDistance
        } // if not culled
        y1 = y2;
    }
    return hit;
}


float HeightfieldModel::elevation(const Point3& osPoint, Vector3& faceNormal) const {
    // const float metersPerQuad   = m_specification.metersPerPixel * m_specification.pixelsPerQuadSide;

    // Fractional part (on grid scale)
    Vector2     f(osPoint.xz() / m_specification.metersPerPixel);

    // Integer pixel position
    Point2int32 P(iFloor(f.x), iFloor(f.y));
    f -= Vector2((float)P.x, (float)P.y);

    // Determine whether we're in the right or left triangle
    //
    //  P ________  > x
    //   |0      2|
    //   | \right |
    //   |   \    |
    //   |     \  |
    //   |2______\|1
    //   v
    //   z

    // Read three pixels
#   define GET(dx, dy) (m_elevationImage->get<Color3>(P + Point2int32(dx, dy), WrapMode::ZERO).r * m_specification.maxElevation)
    float height[3] = {GET(0, 0), GET(1, 1)};
    float weight[3];
    
    // The whole triangle area is always 0.5.  Barycentric coordinates are equal to subtriangle area divided by whole triangle
    // area.  Subtriangle area is half the cross-product of the edge vectors, so the barycentric coordinate in this case
    // is simply the cross product of the edge vectors.  The 2D cross product is (Ax By - Bx Ay).  For each subtriangle,
    // one edge is trivial because it is along an axis.  The other is also trivial: it is vector f.  This means that in the case of a grid,
    // the barycentric coordinates are simply the fractional portion along an axis towards the next cell.

    if (f.x > f.y) {
        // Triangle on the upper-right
        height[2] = GET(1, 0);
        weight[0] = 1.0f - f.x;
        weight[1] = f.y;
        faceNormal = Vector3(-m_specification.metersPerPixel, height[0] - height[2], 0).cross(Vector3(0, height[1] - height[2], +m_specification.metersPerPixel)).direction();
    } else {
        // Triangle on the lower-left
        height[2] = GET(0, 1);
        weight[0] = 1.0f - f.y;
        weight[1] = f.x;
        faceNormal = Vector3(+m_specification.metersPerPixel, height[1] - height[2], 0).cross(Vector3(0, height[0] - height[2], -m_specification.metersPerPixel)).direction();
    }
#   undef GET

    weight[2] = 1.0f - weight[0] - weight[1];

    // Barycentric interpolation
    return height[0] * weight[0] + height[1] * weight[1] + height[2] * weight[2];
}

} // namespace G3D 
