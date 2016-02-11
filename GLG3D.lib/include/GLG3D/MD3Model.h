/**
 \file GLG3D/MD3Model.h

 Quake III MD3 model loading and posing

  \created 2009-01-01
  \edited  2013-03-17
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#ifndef G3D_MD3Model_h
#define G3D_MD3Model_h

#include "G3D/platform.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Matrix3.h"
#include "G3D/Table.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/Texture.h"
#include "GLG3D/UniversalMaterial.h"
#include "GLG3D/Model.h"

namespace G3D {

// forward declare MD3Part
class MD3Part;

class Surface;

/**
    Quake III MD3 model loader.

    Quake 3 uses MD3 models for both characters and non-character objects.  
    Character objects contain three individual "models" inside of them with attachment points.
    
    MD3Models are composed of up to four parts, which are named lower (legs), upper (torso), and head.
    The coordinate frame for each relative to its parent can be specified as part of the pose.
    Each part contains a set of triangle lists.  The triangle lists may have different materials and are 
    key-frame animated. A skin is a set of materials for the triangle lists.  The model is created
    with a default skin, although an alternative skin may be provided as part of the pose.  This allows
    sharing geometry over characters with different appearance.

    It also contains a coordinate frame for a weapon's attachment location.

    See http://bit.ly/acgNj9 for some models
    \beta

    \cite http://icculus.org/homepages/phaethon/q3a/formats/md3format.html
    \cite http://www.misfitcode.com/misfitmodel3d/olh_quakemd3.html

*/
class MD3Model : public Model {
public:

    virtual const String& className() const override;

    enum PartType {
        PART_LOWER,
        PART_UPPER,
        /** Heads are never animated */
        PART_HEAD
    };

    enum {NUM_PARTS = 3, NUM_ANIMATED_PARTS = 2};
   
    /**
        All standard animation types expected to 
        have parameters in the animation.cfg file.
     */
    class AnimType {
    public:
        enum Value {
            BOTH_DEATH1,
            BOTH_DEAD1,
            BOTH_DEATH2,
            BOTH_DEAD2,
            BOTH_DEATH3,
            BOTH_DEAD3,

            UPPER_GESTURE,
            UPPER_ATTACK,
            UPPER_ATTACK2,
            UPPER_DROP,
            UPPER_RAISE,
            UPPER_STAND,
            UPPER_STAND2,

            LOWER_WALKCR,
            LOWER_WALK,
            LOWER_RUN,
            LOWER_BACK,
            LOWER_SWIM,
            LOWER_JUMP,
            LOWER_LAND,
            LOWER_JUMPB,
            LOWER_LANDB,
            LOWER_IDLE,
            LOWER_IDLECR,
            LOWER_TURN };


        static const int NUM_ANIMATIONS = AnimType::LOWER_TURN + 1;
        
        static const int START_BOTH = AnimType::BOTH_DEATH1;
        
        static const int  END_BOTH = AnimType::BOTH_DEAD3;

        static const int START_UPPER = AnimType::UPPER_GESTURE;
        static const int END_UPPER = AnimType::UPPER_STAND2;

        static const int START_LOWER = AnimType::LOWER_WALKCR;
        static const int END_LOWER = AnimType::LOWER_TURN;
    private:
        static const char* toString(int i, Value& v) {
            static const char* str[] = {"BOTH_DEATH1",
                                        "BOTH_DEAD1",
                                        "BOTH_DEATH2",
                                        "BOTH_DEAD2",
                                        "BOTH_DEATH3",
                                        "BOTH_DEAD3",
                                        "UPPER_GESTURE",
                                        "UPPER_ATTACK",
                                        "UPPER_ATTACK2",
                                        "UPPER_DROP",
                                        "UPPER_RAISE",
                                        "UPPER_STAND",
                                        "UPPER_STAND2",
                                        "LOWER_WALKCR",
                                        "LOWER_WALK",
                                        "LOWER_RUN",
                                        "LOWER_BACK",
                                        "LOWER_SWIM",
                                        "LOWER_JUMP",
                                        "LOWER_LAND",
                                        "LOWER_JUMPB",
                                        "LOWER_LANDB",
                                        "LOWER_IDLE",
                                        "LOWER_IDLECR",
                                        "LOWER_TURN"};
            static const Value val[] = {BOTH_DEATH1,
                                        BOTH_DEAD1,
                                        BOTH_DEATH2,
                                        BOTH_DEAD2,
                                        BOTH_DEATH3,
                                        BOTH_DEAD3,

                                        UPPER_GESTURE,
                                        UPPER_ATTACK,
                                        UPPER_ATTACK2,
                                        UPPER_DROP,
                                        UPPER_RAISE,
                                        UPPER_STAND,
                                        UPPER_STAND2,

                                        LOWER_WALKCR,
                                        LOWER_WALK,
                                        LOWER_RUN,
                                        LOWER_BACK,
                                        LOWER_SWIM,
                                        LOWER_JUMP,
                                        LOWER_LAND,
                                        LOWER_JUMPB,
                                        LOWER_LANDB,
                                        LOWER_IDLE,
                                        LOWER_IDLECR,
                                        LOWER_TURN };
            const char* s = str[i];
            if(s) {
                v = val[i];
            }
            return s;
        }

        Value value;
        
        public: 
            G3D_DECLARE_ENUM_CLASS_METHODS(AnimType);
    };

    /* returns true if the animation type effects the lower part */
    static bool affectsLower(AnimType a) {
        if (a < AnimType::UPPER_GESTURE || a > AnimType::UPPER_STAND2) {
            return true;
        } else {
            return false;
        }
    };

    /* returns true if the animation type effects the upper part */
    static bool affectsUpper(AnimType a) {
        return (a < AnimType::LOWER_WALKCR);
    };

    /** A set of materials for a MD3Model. */
    class Skin : public ReferenceCountedObject {        
    private:
        
        Skin() {}

    public:
        /** Maps triList names to materials.  If a material is specified as NULL
            (which corresponds to Quake's common/nodraw), that means "do not draw this triList". */
        typedef Table<String, shared_ptr<UniversalMaterial> >   PartSkin;

        /** Table for each part.  Indices are PartTypes.*/
        Array<PartSkin> partSkin;

        static shared_ptr<Skin> create() {
            return shared_ptr<Skin>(new Skin());
        }

        static shared_ptr<Skin> create
            (const String& commonPath,
            const String& lowerSkin, 
            const String& upperSkin, 
            const String& headSkin);

        /** Loads commonPath + "lower_" + commonSuffix + ".skin", etc. for other parts */
        static shared_ptr<Skin> create
           (const String& commonPath,
            const String& commonSuffix);

        /**
          Format is either: 

           - MD3Model::Skin( <list of part skins> )

          Each part skin is either a .skin file relative to the md3 directory or an Any table mapping a tri list name to a material.  It may have an optional name;
          it is optional but convenient to make this the name of the part. For example:

          <pre>
              MD3Model::Skin(
                 "lower_dragon.skin",
                 "upper_dragon.skin",
                 head {
                   h_cap = NONE, 
                   h_head = UniversalMaterial::Specification {
                      diffuse = "Happy.tga"
                   },
                   h_Visor = NONE,
                   h_Helmet = UniversalMaterial::Specification {
                      diffuse = "Knight2A1.tga"
                   }
                 }
             )
          </pre>
        */
        static shared_ptr<Skin> create(const Any& a);

    private:

        static void loadSkinFile(const String& filename, PartSkin& partSkin);
   };

    /**
        Animation pose based on AnimType and animation time.
        Each animation time ( \a legsTime and \a torsoTime )
        is total time in the current animation which allows for 
        looping based on the parameters in animation.cfg.

        The skins must be the base name of each skin file
        found in the same directory as the model parts.

        Textures for each skin are loaded on first use.
     */
    class Pose : public Model::Pose {
    public:
        SimTime            time[NUM_ANIMATED_PARTS];
        AnimType            anim[NUM_ANIMATED_PARTS];

        AnimType            prevAnim[NUM_ANIMATED_PARTS];
        int                 prevFrame[NUM_ANIMATED_PARTS];
        /** Applying a rotation rotates this part and everything 
          attached to it relative to its parent.  Rotations are typically used to
          make the torso point towards a target or the head in the look direction.*/
        Matrix3             rotation[NUM_PARTS];

        /** If NULL, use the model's default skin */
        shared_ptr<Skin>    skin;

        Pose();
    };

    /** Advances the pose based on this character's animations. */
    void simulatePose(Pose& pose, SimTime dt) const;

    class Specification {
    public:
        /** Directory containing head.md3, upper.md3, lower.md3, torso.md3, and animation.cfg */
        String             directory;

        shared_ptr<Skin>        defaultSkin;

        Specification() {}

        /** 
          Format is:
             <pre>
             MD3Model::Specification {

                // Directory containing the *.md3 files
                directory = "...",

                defaultSkin = MD3Model::Skin( ... )
            }
            </pre>

           or just a string specifying a directory. Example:


           \code
           MD3Model::Specification { 
            defaultSkin = MD3Model::Skin("md3/crusader/models/players/Crusader/lower_default.skin", "md3/crusader/models/players/Crusader/upper_default.skin", "md3/crusader/models/players/Crusader/head_default.skin"); 
            directory = "md3/crusader/models/players/Crusader/"; 
           };
           \endcode

        */
        Specification(const Any& any);
    };
  
    /**A sequence of animation poses that are loaded from the scene file*/

    class PoseSequence {
    private:
        Array<AnimType> poses;
        Array<float>    time;
    public:
        PoseSequence() {}
        /** 
        Format is:
        <pre>
        \code
        md3entity = VisibleEntity { 
            md3pose = MD3::PoseSequence { 
                poses = (LOWER_WALK, UPPER_GESTURE, BOTH_DEATH1);
                times = (4, 4, 6);
            }; 
            
            model = "MD3Model"; 
            visible = true; 
        }; 
        \endcode

        */ 
        PoseSequence(const Any& any);

        Any toAny() const;

        void getPose(float gameTime, Pose& pose) const;
    };

private:


    /** Animation data from animation.cfg */
    class AnimFrame {
    public:
        float   start;
        float   num;
        float   loop;
        float   fps;

        AnimFrame() : start(0), num(0), loop(0), fps(0) {}
        AnimFrame(float s, float n, float l, float f) : start(s), num(n), loop(l), fps(f) {}
    };

    MD3Part*        m_parts[NUM_PARTS];

    AnimFrame       m_animations[AnimType::NUM_ANIMATIONS];

    shared_ptr<Skin> m_defaultSkin;

    String          m_name;

    MD3Model();

    void loadSpecification(const Specification& spec);

    void loadAnimationCfg(const String& filename);

    /** Calculates relative frame number for part */
    float findFrameNum(AnimType animType, SimTime animTime) const;



    void posePart(PartType partType, const Pose& pose, Array<shared_ptr<Surface> >& posedModelArray, 
                  const CFrame& cframe, const CFrame& prevFrame, const shared_ptr<Entity>& entity);

public:

    virtual ~MD3Model();

    static shared_ptr<MD3Model> create(const Specification& spec, const String& name = "");

    static lazy_ptr<Model> lazyCreate(const Specification& s, const String& name = "") {
        return lazy_ptr<Model>( [s, name] { return MD3Model::create(s, name); } );
    }

    virtual const String& name() const override {
        return m_name;
    }
    

    /**
        Poses then adds all available parts to \a posedModelArray.
        Each part is posed based on the animation parameters then
        positioned and rotated based on the appropriate tag according
        to Quake III model standards.

        The lower.md3 part is the based.  The upper.md3 part is attached
        to "tag_torso" in lower.md3.  The head.md3 part is attached to 
        "tag_head" in upper.md3.

        The initial \a cframe transformation is applied to the base 
        lower.md3 part before the whole model is posed.
     */
    void pose(Array<shared_ptr<class Surface> >& posedModelArray, const CoordinateFrame& cframe, const CFrame& previousFrame, const Pose& currentPose = Pose(), const shared_ptr<Entity>& entity = shared_ptr<Entity>());
    void pose(Array<shared_ptr<class Surface> >& posedModelArray, const CoordinateFrame& cframe = CoordinateFrame(), const Pose& currentPose = Pose(), const shared_ptr<Entity>& entity = shared_ptr<Entity>()) {
        pose(posedModelArray, cframe, cframe, currentPose);
    }

    bool intersect(const Ray& r, 
                   const CoordinateFrame& cframe, 
                   const Pose& pose, 
                   float& maxDistance,
                   Model::HitInfo& info = Model::HitInfo::ignore,
                   const shared_ptr<Entity>& entity = shared_ptr<Entity>());

    /** Return the coordinate frame of the tag_weapon; this is where a simulator should place objects carried by the character.*/
    CoordinateFrame weaponFrame(const CoordinateFrame& cframe = CoordinateFrame(), const Pose& pose = Pose()) const;

    const shared_ptr<Skin> defaultSkin() const {
        return m_defaultSkin;
    }
    /**determines which frame in the animation it is currently on and will return a negative
       frame number if it is in the process of blending between animations
     */
    void computeFrameNumbers(const Pose& pose, PartType partType, int& kf0, int& kf1, float& alpha) const;
};


} // namespace G3D

#endif //G3D_MD3Model_h
