#include "G3D/PrefixTree.h"

namespace G3D {
    
PrefixTree::PrefixTree(const String& s) : m_value(s) {}

PrefixTree::~PrefixTree() {}

void PrefixTree::rejectEmptyString(Array<String>& elements) {
    int i = 0;
    while (i < elements.size()) {
        if (elements[i].empty()) {
            elements.remove(i);
        } else {
            ++i;
        }
    }
}


void PrefixTree::compactSplit(const String& s, char delimiter, Array<String>& result) {
    stringSplit(s, delimiter, result);
    rejectEmptyString(result);
}
                

void PrefixTree::compactJoin(Array<String>& elements, char delimiter, String& result) {
    rejectEmptyString(elements);
    result = stringJoin(elements, delimiter);
}


bool PrefixTree::canHaveChildren() {
    return (m_children.size() > 0) || m_value.empty();
}


const shared_ptr<PrefixTree> PrefixTree::childNodeWithPrefix(const String& s) {
   for (shared_ptr<PrefixTree>& child : m_children) {
        if (child->value() == s && child->canHaveChildren()) {
            return child;
        }
    }
    return nullptr;
}


void PrefixTree::insert(const String& s) {
    Array<String> components;
    compactSplit(s, DELIMITER, components);

    // Iterate to *parent* of leaf of existing prefix tree
    // The leaves store the original representation of the element, preserving
    // whitespace, so we should never alter them and instead stop at the parent 
    shared_ptr<PrefixTree> finger = dynamic_pointer_cast<PrefixTree>(shared_from_this());
    int i = 0;
    while (i < components.size()) {
        // Check if any of the finger's children contain the next component
        const shared_ptr<PrefixTree>& next = finger->childNodeWithPrefix(components[i]);
        
        // If not, then we are at the insertion point
        if (isNull(next)) {
            break;
        }

        // If so, continue traversing the prefix tree
        finger = next;
        ++i;
    }

    // Add nodes as necessary, starting at the insertion point
    while (i < components.size()) {
        const String& component(components[i]);

        shared_ptr<PrefixTree> next = PrefixTree::create(component);
        finger->m_children.append(next);
        finger = next;

        ++i;
    }

    // Add leaf node with precise value of string
    finger->m_children.append(std::make_shared<PrefixTree>(s));\
    ++m_size;
}

bool PrefixTree::contains(const String& s) {
    Array<String> components;
    compactSplit(s, DELIMITER, components);

    // Iterate to *parent* of leaf of existing prefix tree
    // The leaves store the original representation of the element, preserving
    // whitespace, so we should never alter them and instead stop at the parent 
    shared_ptr<PrefixTree> finger = dynamic_pointer_cast<PrefixTree>(shared_from_this());
    for (const String& component : components) {
        // Check if any of the finger's children contain the next component
        const shared_ptr<PrefixTree>& next = finger->childNodeWithPrefix(component);

        if (!next) {
            return false;
        }
        finger = next;
    }
    
    // Check that the leaves that extend 
    for (const shared_ptr<PrefixTree>& node : finger->m_children) {
        if (node->isLeaf() && node->value() == s) {
            return true;
        } 
    }
    return false;
}


String PrefixTree::getPathToBranch(shared_ptr<PrefixTree>& branchPoint) {
    shared_ptr<PrefixTree> finger = dynamic_pointer_cast<PrefixTree>(shared_from_this());
                
    Array<String> pathParts;
    while (finger->m_children.size() == 1) {
        pathParts.append(finger->value());
        finger = finger->m_children[0];
    }
    // Omit leaf nodes from the string
    if (finger->m_children.size() > 0) {
        pathParts.append(finger->value());
    }
    
    branchPoint = finger;
    String result;
    compactJoin(pathParts, DELIMITER, result);
    return result;
}

}
