#include <G3D/G3DAll.h>
#include "testassert.h"

Vector3 minCoords(Array<Vector3>& points) {
    Vector3 min = Vector3::maxFinite();
    for(int i = 0; i < points.size(); ++i) {
        for(int j = 0; j < 3; ++j) {
            if(points[i][j] < min[j]) {
                min[j] = points[i][j];
            }
        }
    }
    return min;
}

Vector3 maxCoords(Array<Vector3>& points) {
    Vector3 max = Vector3::minFinite();
    for(int i = 0; i < points.size(); ++i) {
        for(int j = 0; j < 3; ++j) {
            if(points[i][j] > max[j]) {
                max[j] = points[i][j];
            }
        }
    }
    return max;
}

void testIterator(const PointHashGrid<Vector3>& grid, const Array<Vector3>& containedValues) {
    Array<Vector3> entries;
	
    for(PointHashGrid<Vector3>::Iterator iter = grid.begin(); iter != grid.end(); ++iter) {
        entries.append(*iter);
    }
	

    // Check that both arrays contain the same values
    testAssert(entries.size() == containedValues.size());
    for(int i = 0; i < entries.size(); ++i) {
        testAssert(containedValues.contains(entries[i]));
        testAssert(entries.contains(containedValues[i]));
    }
}

void testBoxIterator(const PointHashGrid<Vector3>& grid, const AABox& box, const Array<Vector3>& containedValues) {
    Array<Vector3> entries;
	
    for (PointHashGrid<Vector3>::BoxIterator iter = grid.beginBoxIntersection(box); iter != grid.endBoxIntersection(); ++iter) {
        entries.append(*iter);
    }
	
    // Check that both arrays contain the same values
    testAssert(entries.size() == containedValues.size());
    for(int i = 0; i < entries.size(); ++i) {
        testAssert(containedValues.contains(entries[i]));
        testAssert(entries.contains(containedValues[i]));
    }
}

void testSphereIterator(const PointHashGrid<Vector3>& grid, const Sphere& box, const Array<Vector3>& containedValues) {
    Array<Vector3> entries;

    for (PointHashGrid<Vector3>::SphereIterator iter = grid.beginSphereIntersection(box); iter != grid.endSphereIntersection(); ++iter) {
        entries.append(*iter);
    }
	

    // Check that both arrays contain the same values
    testAssert(entries.size() == containedValues.size());
    for(int i = 0; i < entries.size(); ++i) {
        testAssert(containedValues.contains(entries[i]));
        testAssert(entries.contains(containedValues[i]));
    }
}

void correctPointHashGrid() {
    const int numTestPts = 100;
    const int numIterations = 10000;

    // Gather sphere
    Sphere sphere(Vector3::zero(), 1.0);
    const int avgPtsPerSphere = 4;
    float density = avgPtsPerSphere / sphere.volume();

    // Size of the box we need
    float testVolume = numTestPts / density;
    const Vector3 testExtent = Vector3(1,1,1) * pow(testVolume, 1.0f/3.0f);

    PointHashGrid<Vector3> hashGrid(sphere.radius);
    PointKDTree<Vector3> tree;
	
    Vector3 v;
    for(int i = 0; i < numTestPts; ++i) {
        v = Vector3::random() * testExtent;
        hashGrid.insert(v);
        tree.insert(v);
    }
    tree.balance();

    bool errorFound = false;

    Array<Vector3> hashGridPts;
    Array<Vector3> treePts;

    int iteration = 0;
    while(!errorFound && iteration < numIterations) {
        ++iteration;

        sphere.center = Vector3(uniformRandom(0,1), uniformRandom(0,1), uniformRandom(0,1)) * testExtent;
        hashGridPts.fastClear();
        treePts.fastClear();

        const PointHashGrid<Vector3>::SphereIterator& end = hashGrid.endSphereIntersection();
        for (PointHashGrid<Vector3>::SphereIterator iter = hashGrid.beginSphereIntersection(sphere); iter != end; ++iter) {
            hashGridPts.append(*iter);
        }

        tree.getIntersectingMembers(sphere, treePts);

        for(int i = 0; i < hashGridPts.size(); ++i) {
            if(!treePts.contains(hashGridPts[i])) {
                errorFound = true;
            }
        }
        for(int i = 0; i < treePts.size(); ++i) {
            if(!hashGridPts.contains(treePts[i])) {
                errorFound = true;
            }
        }
    }

    if(errorFound) {
        printf("Discrepancy found:\nSphere center: (%f, %f, %f)\n", sphere.center.x, sphere.center.y, sphere.center.z);
        printf("PointHashGrid found %d elements, PointKDTree found %d elements.\n", hashGridPts.size(), treePts.size());
        printf("\nPointHashGrid found:\n");
        for(int i = 0; i < hashGridPts.size(); ++i) {
            printf("(%f, %f, %f) - ", hashGridPts[i].x, hashGridPts[i].y, hashGridPts[i].z);
            double distance = (hashGridPts[i] - sphere.center).magnitude();
            if(sphere.contains(hashGridPts[i])) {
                printf("IN SPHERE (d = %.4g)\n", distance);
            } else {
                printf("NOT IN SPHERE (d = %.4g)\n", distance);
            }
        }

        printf("\nPointKDTree found:\n");
        for(int i = 0; i < treePts.size(); ++i) {
            printf("(%f, %f, %f) - ", treePts[i].x, treePts[i].y, treePts[i].z);
            double distance = (treePts[i] - sphere.center).magnitude();
            if(sphere.contains(hashGridPts[i])) {
                printf("IN SPHERE (d = %.4g)\n", distance);
            } else {
                printf("NOT IN SPHERE (d = %.4g)\n", distance);
            }
        }
        testAssert(false);
    } else {
        printf("%d iterations complete. No discrepancies found.\n", numIterations);
    }
}

void testSphereIterator() {
    PointHashGrid<Vector3> h(0.1f);
    for (int i = 0; i < 2000; ++i) {
        h.insert(Vector3(uniformRandom(0,1), uniformRandom(0,1), uniformRandom(0,1)));
    }

    for (int i = 0; i < 1000; ++i) {
        Sphere s(Vector3(uniformRandom(0,1), uniformRandom(0,1), uniformRandom(0,1)), 0.1f);

        const PointHashGrid<Vector3>::SphereIterator& end = h.endSphereIntersection();
        for (PointHashGrid<Vector3>::SphereIterator iter = h.beginSphereIntersection(s); iter != end; ++iter) {
            Vector3 v = *iter;
            testAssertM(s.contains(v), "SphereIterator returned a point that was not in the sphere");
        }
    }
}

void testPointHashGrid() {
    testSphereIterator();
    correctPointHashGrid();

    Array<Vector3> vec3Array;
    vec3Array.append(Vector3(0.0, 0.0, 0.0));
    vec3Array.append(Vector3(1.0, 0.0, 0.0));
    vec3Array.append(Vector3(0.0, 1.0, 0.0));
    vec3Array.append(Vector3(0.0, 0.0, 1.0));
    vec3Array.append(Vector3(1.0, 1.0, 0.0));
    vec3Array.append(Vector3(1.0, 0.0, 1.0));
    vec3Array.append(Vector3(0.0, 1.0, 1.0));
    vec3Array.append(Vector3(1.0, 1.0, 1.0));

    PointHashGrid<Vector3> grid(0.5f);

    // Test insert - one element
    for(int i = 0; i < vec3Array.size(); ++i) {
        grid.insert(vec3Array[i]);
    }

    // Test size
    testAssert(vec3Array.size() == grid.size());

    // Test conservativeBoxBounds
    AABox arrayBox(minCoords(vec3Array), maxCoords(vec3Array));
    testAssert(arrayBox == grid.conservativeBoxBounds());

    // Test remove() and contains()	
    for(int i = 0; i < vec3Array.size(); ++i) {
        testAssert(grid.contains(vec3Array[i]));
        testAssert(grid.remove(vec3Array[i]) == true);
    }
    testAssert(grid.size() == 0);
    testAssert(!grid.contains(Vector3(-1.0, -1.0, -1.0)));
    testAssert(grid.remove(Vector3(-1.0, -1.0, -1.0)) == false);
	

    // Test insert - array of elements
    grid.insert(vec3Array);

    // Test Iterator
    testIterator(grid, vec3Array);


    // Test BoxIterator
    testBoxIterator(grid, arrayBox, vec3Array);

    // Test SphereIterator
    Array<Vector3> unitVectors;
    unitVectors.append(Vector3::zero());
    unitVectors.append(Vector3::unitX());
    unitVectors.append(Vector3::unitY());
    unitVectors.append(Vector3::unitZ());
    testSphereIterator(grid, Sphere(Vector3::zero(), 1.0f), unitVectors);

    // Test CellIterator
	
    int entriesFound = 0;
    const PointHashGrid<Vector3>::CellIterator& end = grid.endCells();
    for(PointHashGrid<Vector3>::CellIterator iter = grid.beginCells(); iter != end; ++iter) {
        testAssert(iter->size() > 0);
        entriesFound += iter->size();
    }
    testAssert(entriesFound == vec3Array.size());
	
    // Test clear
    grid.clear();
    testAssert(grid.size() == 0);
}

void getVertices(shared_ptr<ArticulatedModel> model, Array<Point3>& vertexArray) {

  const Array<ArticulatedModel::Geometry*> geomArray = model->geometryArray();
    for (int g = 0; g < geomArray.size(); ++g) {
      ArticulatedModel::Geometry* geom = geomArray[g];
      for (int v = 0; v < geom->cpuVertexArray.size(); ++v) {
	vertexArray.append(geom->cpuVertexArray.vertex[v].position);
      }
    }

}

Vector3 min(Array<Vector3>& v) {
    Vector3 mn = Vector3::maxFinite();
    for(int i = 0; i < v.size(); ++i) {
        mn = mn.min(v[i]);
    }
    return mn;
}

Vector3 max(Array<Vector3>& v) {
    Vector3 mx = Vector3::minFinite();
    for(int i = 0; i < v.size(); ++i) {
        mx = mx.max(v[i]);
    }
    return mx;
}

void perfPointHashGrid() {
    const int numSpheres = 100000;

    /*
    // Gather sphere
    const int avgPtsPerSphere = 4;
    float density = avgPtsPerSphere / sphere.volume();

    // Size of the box we need
    float testVolume = numTestPts / density;
    const Vector3 testExtent = Vector3(1,1,1) * pow(testVolume, 1.0f/3.0f);
    */
	
    Array<Vector3> v;
    const String& filename = System::findDataFile("cow.ifs");
    shared_ptr<ArticulatedModel> m = ArticulatedModel::fromFile(filename);
    getVertices(m, v);
    int numTestPts = v.size();
    Vector3 minCoords = min(v);
    Vector3 maxCoords = max(v);
    Sphere sphere(Vector3::zero(), ((maxCoords - minCoords).average()) / 100.0f);
    PointHashGrid<Vector3> hashGrid(sphere.radius * 2.0f);
    PointKDTree<Vector3> tree;

    Stopwatch hashGridInsert;
    Stopwatch treeInsert;
    double hashGridInsertTime = 0.0;
    double treeInsertTime = 0.0;

    hashGridInsert.tick();
    hashGrid.insert(v);
    hashGridInsert.tock();
    hashGridInsertTime = hashGridInsert.elapsedTime();
    treeInsert.tick();
    tree.insert(v);
    treeInsert.tock();
    treeInsertTime = treeInsert.elapsedTime();

    Stopwatch treeBalance;

    treeBalance.tick();
    tree.balance();
    treeBalance.tock();

    //testAssert(tree.size() == hashGrid.size());
    printf("%d elements\n", numTestPts);
    printf("Tree insert time:               %f s (%f us / element)\n", treeInsertTime, 1e6*treeInsertTime / numTestPts);
    printf("Tree balance time:              %f s (%f us / element)\n", treeBalance.elapsedTime(), 1e6*treeBalance.elapsedTime() / numTestPts);
    printf("Total tree insert/balance time: %f s (%f us / element)\n", treeInsertTime + treeBalance.elapsedTime(), 1e6*(treeInsertTime + treeBalance.elapsedTime()) / numTestPts);
    printf("HashGrid insert time:           %f s (%f us / element)\n", hashGridInsertTime, hashGridInsertTime * 1e6 / numTestPts);

    Stopwatch hashGridTimer;
    Stopwatch treeTimer;
    Array<Vector3> pos;
    pos.resize(numSpheres);

    for (int i = 0; i < numSpheres; ++i) {
        pos[i] = v.randomElement();
    }

    Vector3 sum = Vector3::zero();
    int countHash = 0;

    // Test PointHashGrid
    hashGridTimer.tick();
    for(int i = 0; i < numSpheres; ++i) {
        sphere.center = pos[i];

        const PointHashGrid<Vector3>::SphereIterator& end = hashGrid.endSphereIntersection();
        for (PointHashGrid<Vector3>::SphereIterator iter = hashGrid.beginSphereIntersection(sphere); iter != end; ++iter) {
            sum += *iter;
            ++countHash;
        }
    }
    hashGridTimer.tock();

    // Test AABSP
    sum = Vector3::zero();
    Array<Vector3> inSphere;
    int count = 0;
    treeTimer.tick();
    for (int i = 0; i < numSpheres; ++i) {
        sphere.center = pos[i];
        inSphere.fastClear();
        tree.getIntersectingMembers(sphere, inSphere);
        for (int i = 0; i < inSphere.size(); ++i) {
            sum += inSphere[i];
            ++count;
        }
    }
    treeTimer.tock();
    testAssertM(iAbs(countHash - count) <= iMax(countHash, count) * 0.001, 
                  format("Fetched different numbers of points. PointHashGrid = %d, PointKDTree = %d",
                         countHash, count));

    printf("\nSphere Intersection (%d trials, fetched %dk points)\n", numSpheres, count / 1000);
    printf("    class         1M elt-time        time/elt\n");
    printf("PointKDTree  %10f s  %10f us\n",              treeTimer.elapsedTime(),     treeTimer.elapsedTime() * 1e6 / count);
    printf("PointHashGrid   %10f s  %10f us (%.3gX faster)\n", hashGridTimer.elapsedTime(), hashGridTimer.elapsedTime() * 1e6 / count,
           treeTimer.elapsedTime()/hashGridTimer.elapsedTime());
    printf("\nPointHashGrid performance: max bucket size = %d, average length = %f\n", hashGrid.debugGetDeepestBucketSize(), hashGrid.debugGetAverageBucketSize());
}
