#ifndef G3D_PrefixTree_h
#define G3D_PrefixTree_h

#include "G3D/Array.h"
#include "G3D/stringutils.h"
#include "GLG3D/GuiText.h"

namespace G3D {
class PrefixTree : public std::enable_shared_from_this<PrefixTree> {
protected:
    /** Number of leaf nodes */
    int                             m_size = 0;
    const String                    m_value;        
    Array<shared_ptr<PrefixTree>>   m_children;

    /** Mutates array, omitting elements that are the empty string */
    static void rejectEmptyString(Array<String>& elements) {
        int i = 0;
        while (i < elements.size()) {
            if (elements[i].empty()) {
                elements.remove(i);
            } else {
                ++i;
            }
        }
    }

    static void compactSplit(const String& s, char delimiter, Array<String>& result) {
        result = stringSplit(s, delimiter);   
        rejectEmptyString(result);
    }
                
    /** Note: modifies input array */
    static void compactJoin(Array<String>& elements, char delimiter, String& result) {
        rejectEmptyString(elements);
        result = stringJoin(elements, delimiter);
    }
        
    const shared_ptr<PrefixTree> childNodeWithPrefix(const String& s);

    bool canHaveChildren();

public:

    static const char DELIMITER = ' ';

    PrefixTree(const String& s = DELIMITER);
    virtual ~PrefixTree();
        
    const String& value() const { return m_value; }
    const Array<shared_ptr<PrefixTree>>& children() const { return m_children; }

    void insert(const String& s);
    bool contains(const String& s);

    /**
      If the node is a leaf, then its value is the full inserted value.
      Roughly, the result of joining all the prefixes on the path to the
      leaf; specifically, the String that was passed to PrefixTree::insert()
    */
    bool isLeaf() const { return m_children.size() == 0; };
    String getPathToBranch(shared_ptr<PrefixTree>& branchPoint);
    int size() const { return m_size; }
        
    static shared_ptr<PrefixTree> create(const String& s = DELIMITER) {
        return std::make_shared<PrefixTree>(s);
    }

    template <class T> // Generic to allow both GuiText, String
    static shared_ptr<PrefixTree> create(const Array<T>& elements) {
        shared_ptr<PrefixTree> tree = PrefixTree::create();
        for (const String& s : elements) {
            tree->insert(s);
        }
        return tree;
    }
};
}


#endif