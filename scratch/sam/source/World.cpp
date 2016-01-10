


#include "World.h"

World::World() : m_mode(TRACE) {
    begin();

    lightArray.append(Light::point("Light1", Vector3(0, 10, 4), Color3::white() * 1200));
    lightArray.append(Light::point("Light2", Vector3(2.6f, 6.9f,  6.6f), Color3::fromARGB(0xffe5bd) * 1000));

    ambient = Radiance3::fromARGB(0x304855) * 0.3f;
	
    Any teapot = PARSE_ANY
        ( ArticulatedModel::Specification {
            filename = "teapot/teapot.obj";
            scale = 0.01;
            stripMaterials = true;
            preprocess = 
                ( setMaterial(all(),
                             UniversalMaterial::Specification {
                                 specular = Color3(0.2f);
                                 //shininess = mirror();
								 glossy = Color4(0.0f, 0.0f, 0.0f, 1);
                                 lambertian = Color3(0.7f, 0.5f, 0.1f);
                             });
                 );
         } );

    insert(ArticulatedModel::create(teapot), CFrame::fromXYZYPRDegrees(-0.5f, -0.6f, 0.7f, 50));
	//insert(ArticulatedModel::create(teapot), CFrame::fromXYZYPRDegrees(-0.5f, -0.6f, 0.3f, 50));
    Any sphereOutside = PARSE_ANY 
        ( ArticulatedModel::Specification {
            filename = "sphere.ifs";
            scale = 0.3;
            preprocess =
                ( setTwoSided(all(), true);
                    setMaterial(all(),
                              UniversalMaterial::Specification {
                                  specular = Color3(0.2f);
                                  //shininess = mirror();
								 glossy = Color4(0.0f, 0.0f, 0.0f, 1);
                                  lambertian = Color3(0.0f);
                                  etaTransmit = 1.3f;
                                  etaReflect = 1.0f;
//                                  transmissive = Color3(0.4f, 0.7f, 0.9f);
                                  transmissive = Color3(0.1f, 0.2f, 0.1f);
                              });
                  );
          });

    Any sphereInside = PARSE_ANY 
        ( ArticulatedModel::Specification {
            filename = "sphere.ifs";
            scale = -0.3;
            preprocess =
                ( setTwoSided( all(), true);
                    setMaterial(all(),
                              UniversalMaterial::Specification {
                                  specular = Color3(0.1f);
                                  //shininess = mirror();
								  glossy = Color4(0.0f, 0.0f, 0.0f, 1);
                                  lambertian = Color3(0.0f);
                                  etaReflect = 1.3f;
                                  etaTransmit = 1.0f;
                                  transmissive = Color3(1.0f);
                              });
                  );
          });
	
    Any wall = PARSE_ANY 
        ( ArticulatedModel::Specification {
            filename = "squarex8.ifs";
            scale = 3;
            preprocess =
                ( setTwoSided(all(), true);
                    setMaterial( all(),
                              UniversalMaterial::Specification {
                                  specular = Color3(0.0f);
                                  //shininess = mirror();
								glossy = Color4(0.0f, 0.0f, 0.0f, 1);
                                  lambertian = Color3(1.0f, 0.8f, 0.8f);
                                  etaReflect = 1.0f;
                                  etaTransmit = 1.0f;
                                  transmissive = Color3(0.0f);
                              });
                  );
          });
	
    insert(ArticulatedModel::create(sphereOutside), CFrame::fromXYZYPRDegrees(0.3f, -0.2f, 0.5f, 0));
    insert(ArticulatedModel::create(wall), CFrame::fromXYZYPRDegrees(-0.8f, -0.2f, -1.5f, 0));
    insert(ArticulatedModel::create(wall), CFrame::fromXYZYPRDegrees( 2.3f, -0.2f, -1.5f, 0));
    insert(ArticulatedModel::create(sphereInside),  CFrame::fromXYZYPRDegrees(19.7f, 0.2f, -1.1f, 70));
	
/*    Any sponza = PARSE_ANY
        ( ArticulatedModel::Specification {
            filename = "dabrovic_sponza/sponza.zip/sponza.obj";
          } );
    Stopwatch timer;
    insert(ArticulatedModel::create(sponza), Vector3(8.2f, -6, 0));
    timer.after("Sponza load");
	*/
    end();
}



void World::begin() {
    debugAssert(m_mode == TRACE);
    m_surfaceArray.clear();
    m_mode = INSERT;
}


/*void printIfWanted(const Tri T, const CPUVertexArray& cpuVertexArray) {
	Triangle t = T.toTriangle(cpuVertexArray);
		if ((t.vertex(0).fuzzyEq(Vector3(0.7f, 1.3f, -1.5f)) && t.vertex(1).fuzzyEq(Vector3(-2.3f, 1.3f, -1.5f)) && t.vertex(2).fuzzyEq(Vector3(-2.3f, -1.7f, -1.5f))) ||
			(t.vertex(0).fuzzyEq(Vector3(0.7f, 1.3f, -1.5f)) && t.vertex(1).fuzzyEq(Vector3(-2.3f, -1.7f, -1.5f)) && t.vertex(2).fuzzyEq(Vector3(.7f, -1.7f, -1.5f))) ||
			(t.vertex(0).fuzzyEq(Vector3(3.8f, 1.3f, -1.5f)) && t.vertex(1).fuzzyEq(Vector3(0.8f, 1.3f, -1.5f)) && t.vertex(2).fuzzyEq(Vector3(0.8f, -1.7f, -1.5f))) ||
			(t.vertex(0).fuzzyEq(Vector3(3.8f, 1.3f, -1.5f)) && t.vertex(1).fuzzyEq(Vector3(0.8f, -1.7f, -1.5f)) && t.vertex(2).fuzzyEq(Vector3(3.8f, -1.7f, -1.5f)))) {
		debugPrintf("[(%f, %f, %f), (%f, %f, %f), (%f, %f, %f)]\n", t.vertex(0).x, t.vertex(0).y, t.vertex(0).z, t.vertex(1).x, t.vertex(1).y, t.vertex(1).z, t.vertex(2).x, t.vertex(2).y, t.vertex(2).z);
		}
}*/
void World::printTris() {
	/**[(0.700000, 1.300000, -1.500000), (-2.300000, 1.300000, -1.500000), (-2.300000, -1.700000, -1.500000)]
	[(0.700000, 1.300000, -1.500000), (-2.300000, -1.700000, -1.500000), (0.700000, -1.700000, -1.500000)]
	[(3.800000, 1.300000, -1.500000), (0.800000, 1.300000, -1.500000), (0.800000, -1.700000, -1.500000)]
	[(3.800000, 1.300000, -1.500000), (0.800000, -1.700000, -1.500000), (3.800000, -1.700000, -1.500000)]*/
	/*for (int i=0;i<m_triTree.size();++i) {
		printIfWanted(m_triTree[i], m_triTree.cpuVertexArray());
	}*/
}

void World::insert(const shared_ptr<ArticulatedModel>& model, const CFrame& frame) {
    Array<shared_ptr<Surface> > posed;
    model->pose(posed, frame);
    for (int i = 0; i < posed.size(); ++i) {
        insert(posed[i]);
    }
}

void World::insert(const shared_ptr<Surface>& m) {
    debugAssert(m_mode == INSERT);
    m_surfaceArray.append(m);
}


void World::end() {
    m_triTree.setContents(m_surfaceArray);
    debugAssert(m_mode == INSERT);
    m_mode = TRACE;
}


bool World::lineOfSight(const Vector3& v0, const Vector3& v1) const {
    debugAssert(m_mode == TRACE);
    
    Vector3 d = v1 - v0;
    float len = d.length();
    Ray ray = Ray::fromOriginAndDirection(v0, d / len);
    float distance = len;
    Tri::Intersector intersector;

    // For shadow rays, try to find intersections as quickly as possible, rather
    // than solving for the first intersection
    static const bool exitOnAnyHit = true, twoSidedTest = true;
    return ! m_triTree.intersectRay(ray, intersector, distance, exitOnAnyHit, twoSidedTest);

}


shared_ptr<Surfel> World::intersect(const Ray& ray, float& distance) const {
    debugAssert(m_mode == TRACE);

    return m_triTree.intersectRay(ray, distance);
}

