/*
  TODO:
  
  - Do tangents really need a sign bit?
*/

/**
   Computes missing normal and tangent space data and merges vertices
   with identical attributes.

   \param vertex    Vertex positions
   \param normal    Per-vertex normals; empty if they should be computed
   \param texCoord  Zero-length if there are none
   \param tangent   Tangents, with sign in the w component.  Empty if they should be computed.
   \param index     Index array for every triangle in this part, regardless of material
   \param indexRemap <code>oldVertex[i] == vertex[newIndex[i]]</code>
 */
void completeGeometry
(Array<Point3>&  vertex,
 Array<Vector3>& normal,
 Array<Point2>&  texCoord,
 Array<Vector4>& tangent,
 Array<int>&     index,
 Array<int>&     indexRemap,
 float normalSmoothingAngleRadians) {

    if (needsNormals || needsTangents) {
        computeAdjacency(index, adjacency);

        if (needsNormals) {
            computeNormals(normalSmoothingAngleRadians, adjacency, normal);
        } 

        if (needsTangents && hasTexCoords) {
            computeTexCoords(adjacency, texCoord, tangent);
        }
    }

    mergeIdenticalVertices(vertex, normal, texCoord, index);
}


void ArticulatedModel::addPart(const std::string& name, geometry);
void ArticulatedModel::Part::addTriList(const Material::Specification& material, PrimitiveType primitive, const Array<int>& index, bool twoSided);
