#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

/**
 An KDTree that can render itself for debugging purposes.
 */
class VisibleBSP : public KDTree<Vector3> {
protected:

    void drawPoint(RenderDevice* rd, const Vector2& pt, float radius, const Color3& col) {
        Draw::rect2D(Rect2D::xywh(pt.x - radius, pt.y - radius, radius * 2, radius * 2), rd, col);
    }

    void drawNode(RenderDevice* rd, Node* node, float radius, int level) {
#if 0        
        Color3 color = Color3::white();

        // Draw the points at this node
        for (int i = 0; i < node->valueArray.size(); ++i) {
            const Vector3& pt = node->valueArray[i]->value;
            drawPoint(rd, pt.xy(), radius, Color3::cyan());
        }

        if (node->splitAxis != 2) {
            // Draw the splitting plane
            const AABox& bounds = node->splitBounds;
            Vector2 v1 = bounds.low().xy();
            Vector2 v2 = bounds.high().xy();

            // Make the line go horizontal or vertical based on the split axis
            v1[node->splitAxis] = node->splitLocation;
            v2[node->splitAxis] = node->splitLocation;

            rd->setColor(color);
            rd->beginPrimitive(PrimitiveType::LINES);
                rd->sendVertex(v1);
                rd->sendVertex(v2);
            rd->endPrimitive();
        }

        // Shrink radius
        float nextRad = max(0.5f, radius / 2.0f);

        for (int i = 0;i < 2; ++i) {
            if (node->child[i]) {
                drawNode(rd, node->child[i], nextRad, level + 1);
            }
        }
#endif
    }

public:
    VisibleBSP(int w, int h) {
        int N = 200;
        for (int i = 0; i < N; ++i) {
            insert(Vector3(uniformRandom(0, (float)w), uniformRandom(0, (float)h), 0));
        }
        balance();
    }

    /**
     Draw a 2D projected version; ignore splitting planes in z
     */
    void render2D(RenderDevice* rd) {
        rd->push2D();
            drawNode(rd, root, 20, 0);
        rd->pop2D();
    }
    
};

static void testSerialize() {
    KDTree<Vector3> tree;
    int N = 1000;

    for (int i = 0; i < N; ++i) {
        tree.insert(Vector3::random());
    }
    tree.balance();

    // Save the struture
    BinaryOutput b("test-bsp.dat", G3D_LITTLE_ENDIAN);
    tree.serializeStructure(b);
    b.commit();

}


static void testBoxIntersect() {

	KDTree<Vector3> tree;

	// Make a tree containing a regular grid of points
	for (int x = -5; x <= 5; ++x) {
		for (int y = -5; y <= 5; ++y) {
			for (int z = -5; z <= 5; ++z) {
				tree.insert(Vector3((float)x, (float)y, (float)z));
			}
		}
	}
	tree.balance();

	AABox box(Vector3(-1.5, -1.5, -1.5), Vector3(1.5, 1.5, 1.5));

	KDTree<Vector3>::BoxIntersectionIterator it = tree.beginBoxIntersection(box);
	const KDTree<Vector3>::BoxIntersectionIterator end = tree.endBoxIntersection();

	int hits = 0;
	while (it != end) { 
		const Vector3& v = *it;

		testAssert(box.contains(v));

		++hits;
		++it;
	}

	testAssertM(hits == 3*3*3, "Wrong number of intersections found in testBoxIntersect for KDTree");
}


void perfKDTree() {

    Array<AABox>                array;
    KDTree<AABox>            tree;
    
    const int NUM_POINTS = 1000000;
    
    for (int i = 0; i < NUM_POINTS; ++i) {
        Vector3 pt = Vector3(uniformRandom(-10, 10), uniformRandom(-10, 10), uniformRandom(-10, 10));
        AABox box(pt, pt + Vector3(.1f, .1f, .1f));
        array.append(box);
        tree.insert(box);
    }

    RealTime t0 = System::time();
    tree.balance();
    RealTime t1 = System::time();
    printf("KDTree<AABox>::balance() time for %d boxes: %gs\n\n", NUM_POINTS, t1 - t0);

    uint64 bspcount = 0, arraycount = 0, boxcount = 0;

    // Run twice to get cache issues out of the way
    for (int it = 0; it < 2; ++it) {
        Array<Plane> plane;
        plane.append(Plane(Vector3(-1, 0, 0), Vector3(3, 1, 1)));
        plane.append(Plane(Vector3(1, 0, 0), Vector3(1, 1, 1)));
        plane.append(Plane(Vector3(0, 0, -1), Vector3(1, 1, 3)));
        plane.append(Plane(Vector3(0, 0, 1), Vector3(1, 1, 1)));
        plane.append(Plane(Vector3(0,-1, 0), Vector3(1, 3, 1)));
        plane.append(Plane(Vector3(0, 1, 0), Vector3(1, -3, 1)));

        AABox box(Vector3(1, 1, 1), Vector3(3,3,3));

        Array<AABox> point;

        System::beginCycleCount(bspcount);
        tree.getIntersectingMembers(plane, point);
        System::endCycleCount(bspcount);

        point.clear();

        System::beginCycleCount(boxcount);
        tree.getIntersectingMembers(box, point);
        System::endCycleCount(boxcount);

        point.clear();

        System::beginCycleCount(arraycount);
        for (int i = 0; i < array.size(); ++i) {
            if (! array[i].culledBy(plane)) {
                point.append(array[i]);
            }
        }
        System::endCycleCount(arraycount);
    }

    printf("KDTree<AABox>::getIntersectingMembers(plane) %g Mcycles\n"
           "KDTree<AABox>::getIntersectingMembers(box)   %g Mcycles\n"
           "Culled by on Array<AABox>                       %g Mcycles\n\n", 
           bspcount / 1e6, 
           boxcount / 1e6,
           arraycount / 1e6);
}

class IntersectCallback {
public:
    void operator()(const Ray& ray, const Triangle& tri, float& distance) {
        float d = ray.intersectionTime(tri);
        if (d > 0 && d < distance) {
            distance = d;
        }
    }
};


void extractTriangles(shared_ptr<ArticulatedModel> model, Array<Point3>& vertexArray, Array<int>& indexArray) {
  const Array<ArticulatedModel::Geometry*> geomArray = model->geometryArray();
  const Array<ArticulatedModel::Mesh*>     meshArray = model->meshArray();
    for (int i = 0; i < geomArray.size(); ++i) {
      int offset = vertexArray.size();
      ArticulatedModel::Geometry* geom = geomArray[i];
      for (int j = 0; j < geom->cpuVertexArray.size(); ++j) {
          vertexArray.append(geom->cpuVertexArray.vertex[j].position);
      }

      for (int m = 0; m < meshArray.size(); ++m) {
	const ArticulatedModel::Mesh* mesh = meshArray[m];
	if (mesh->geometry == geom) {
	  for (int j = 0; j < mesh->cpuIndexArray.size(); ++j) {
	    indexArray.append(mesh->cpuIndexArray[j] + offset);
	  }
	}
      }
    }
    
}

void testRayIntersect() {
    KDTree<Triangle> tree;

    Array<int> index;
    Array<Point3> vertex;
    printf(" (load model, ");
    fflush(stdout);

    shared_ptr<ArticulatedModel> model = ArticulatedModel::fromFile(System::findDataFile("cow.ifs"));
    extractTriangles(model, vertex, index);
    
    for (int i = 0; i < index.size(); i += 3) {
        int i0 = index[i];
        int i1 = index[i + 1];
        int i2 = index[i + 2];
        tree.insert(Triangle(vertex[i0], vertex[i1], vertex[i2]));
    }
    printf("balance tree, ");
    fflush(stdout);
    tree.balance();

    Vector3 origin = Vector3(0, 5, 0);
    IntersectCallback intersectCallback;
    printf("raytrace, ");
    fflush(stdout);
    for (int i = 0; i < 4000; ++i) {
        // Cast towards a random point near the cow surface
        Vector3 target = vertex.randomElement() + Vector3::random() * 0.0001f;
        Ray ray = Ray::fromOriginAndDirection(origin, (target - origin).direction());

        // Exhaustively test against each triangle
        float exhaustiveDistance = (float)inf();
        {
            const KDTree<Triangle>::Iterator& end = tree.end();
            KDTree<Triangle>::Iterator it = tree.begin();

            while (it != end) {
                const Triangle& tri = *it;
                float d = ray.intersectionTime(tri);
                if (d > 0 && d < exhaustiveDistance) {
                    exhaustiveDistance = d;
                }
                ++it;
            }
        }

        // Test using the ray iterator
        float treeDistance = (float)inf();
        tree.intersectRay(ray, intersectCallback, treeDistance, true);

        float treeDistance2 = (float)inf();
        tree.intersectRay(ray, intersectCallback, treeDistance2, false);

        testAssertM(fuzzyEq(treeDistance, exhaustiveDistance),
                     format("KDTree::intersectRay found a point at %f, "
                            "exhaustive ray intersection found %f.",
                            treeDistance, exhaustiveDistance));

        testAssertM(fuzzyEq(treeDistance2, exhaustiveDistance),
                     format("KDTree::intersectRay found a point at %f, "
                            "exhaustive ray intersection found %f.",
                            treeDistance2, exhaustiveDistance));
    }
    printf("done) ");
}


void testKDTree() {
    printf("KDTree ");
    
    testRayIntersect();
    testBoxIntersect();
    testSerialize();

    printf("passed\n");
}
