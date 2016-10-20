#if 0

/** 
 CubeMap<Image3>::Ref cube = CubeMap<Image3>::create(...);

 CubeMap<Image3>::Ref im = CubeMap<Image3>::create(System::findDataFile("test/testcube_*.jpg"));

*/
template<class Image>
class CubeMap : public ReferenceCountedObject {
public:
    typedef shared_ptr< class CubeMap<Image> > Ref;

protected:

    int                         m_width;
    Array<typename shared_ptr<Image> >  m_imageArray;

    CubeMap() {}

    void init(const Array<typename shared_ptr<Image> >& imageArray) {
        m_imageArray = imageArray;
        debugAssert(imageArray.size() == 6);
        m_width = m_imageArray[0]->width();
        for (int i = 0; i < 6; ++i) {
            debugAssert(m_imageArray[i]->width()  == m_width);
            debugAssert(m_imageArray[i]->height() == m_width);
        }
    }

public:

    static Ref create(const std::string& fileSpec) {
        // TODO
        return NULL;
    }

    /** Retains references to the underlying images. */
    static Ref create(const Array<typename shared_ptr<Image> >& imageArray) {
        Ref c = new CubeMap<Image>();
        c->init(imageArray);
        return c;
    }

    typename Image::ComputeType bilinear(const Vector3& v) const;

    typename Image::ComputeType nearest(const Vector3& v) const;

    /** Return the image representing one face. */
    const typename shared_ptr<Image> face(CubeFace f) const {
        return m_imageArray[f];
    }
    
    /** Returns the width of one side, which must be the same as the height. */
    int width() const {
        return m_width;
    }

    /** Returns the height of one side in pixels, which must be the same as the height. */
    int height() const {
        // Width and height are the same in this class.
        return m_width;
    }

};

#endif
