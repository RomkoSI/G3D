
#ifndef testTreeBuilder_h
#define testTreeBuilder_h

#include "String.h"

class testTreeBuilder : public DepthFirstTreeBuilder<String> { 
private:
    Array<String> tree;
    int depth;

public: 

    String output;

    testTreeBuilder() : DepthFirstTreeBuilder() {
        depth = 0;
    };
    
    virtual ~testTreeBuilder() {}; 

    virtual void enterChild(const String& node) {       
        tree.push(node);

        String indent = "";
        for (int i = 0; i < depth; ++i) {
            indent += " ";
        }

        output += (indent + "-" + node + "\n");
        depth += 1;
    };
    
    virtual void goToParent() {
        if(tree.size() != 0 ) {
            depth -= 1;
            tree.pop();
        } else {
            debugPrintf("Cannot go to parent of empty tree\n");
        }
    }; 

    void clear() {
        depth = 0;
        output.clear();
    }
};

#endif 