/**
\file ArticulatedModelSpecificationEditorDialog.cpp

\maintainer Morgan McGuire, http://graphics.cs.williams.edu
\author Sam Donow

\created 2014-06-26
\edited  2015-01-03
*/

#include "GLG3D/ArticulatedModelSpecificationEditorDialog.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiTabPane.h"
#include "G3D/FileSystem.h"
#include "G3D/ParseOBJ.h"

namespace G3D {

ArticulatedModelSpecificationEditorDialog::ArticulatedModelSpecificationEditorDialog(OSWindow* osWindow, const shared_ptr<GuiTheme>& skin, const String& note) : 
    GuiWindow("", skin, Rect2D::xywh(400, 100, 10, 10), GuiTheme::DIALOG_WINDOW_STYLE, HIDE_ON_CLOSE), 
    ok(false), m_osWindow(osWindow) {

    GuiPane* rootPane = pane();
    GuiTabPane* tabPane = rootPane->addTabPane();

    GuiPane* sPane = tabPane->addTab("standard");
    {
        fileNameBox = sPane->addTextBox("filename: ", &m_spec.filename, GuiTextBox::IMMEDIATE_UPDATE);
        fileNameBox->setSize(Vector2(min(osWindow->width() - 100.0f, 600.0f), fileNameBox->rect().height()));
        //textBox->setFocused(true);

        scaleBox = sPane->addNumberBox<float>("scale: ", &m_spec.scale);
        scaleBox->setSize(Vector2(min(osWindow->width() - 100.0f, 200.0f), fileNameBox->rect().height()));
        scaleBox->setFocused(true);

        useAutoScale = true;
        GuiCheckBox* autoScaleBox        = sPane->addCheckBox("Autoscale (m). If unchecked, scales directly.", &useAutoScale);
        autoScaleBox->moveRightOf(scaleBox);

        translation = NONE;

        sPane->addLabel("Translation:");   
        sPane->addRadioButton("None", NONE  , &translation);
        sPane->addRadioButton("Center to Origin", CENTER, &translation); 
        sPane->addRadioButton("Base to Origin",  BASE  , &translation); 

        saveButton   = sPane->addButton("Save");
        cancelButton = sPane->addButton("Cancel");
        cancelButton->moveRightOf(saveButton);
        okButton     = sPane->addButton("Ok");
        okButton->moveRightOf(cancelButton);

        okButton->setEnabled(true);

        if (note != "") {
            sPane->addLabel(note);
        }

        pack();
    }
    GuiPane* objPane = tabPane->addTab("obj");
    objPane->setSize(Vector2(sPane->rect().width(), sPane->rect().height()));
    {
        objPane->addLabel("TexCoord1 Mode:");

        texCoord1Mode = ParseOBJ::Options::NONE;

        objPane->addRadioButton("NONE", ParseOBJ::Options::NONE, &texCoord1Mode);
        objPane->addRadioButton("UNPACK_FROM_TEXCOORD0_Z", ParseOBJ::Options::UNPACK_FROM_TEXCOORD0_Z, &texCoord1Mode);
        objPane->addRadioButton("TEXCOORD0_ZW", ParseOBJ::Options::TEXCOORD0_ZW, &texCoord1Mode);

        stripRefraction = false;
        objPane->addCheckBox("stripRefraction", &stripRefraction);

        pack();
    }
}


void ArticulatedModelSpecificationEditorDialog::finalizeSpecification() {

    /** Computes AutoScale by loading model and computing bounding box */
    if (useAutoScale) {
        AABox box;
        const float oldScale = m_spec.scale;
        m_spec.scale = 1.0f;
        shared_ptr<ArticulatedModel> am = ArticulatedModel::create(m_spec);
        am->getBoundingBox(box);
        Vector3 extent = box.extent();
        
        // Scale to X units
        const float scale = 1.0f / max(extent.x, max(extent.y, extent.z));
        
        if (scale <= 0) {
            m_spec.scale = 1;
        } else if (! isFinite(scale)) {
            m_spec.scale = 1;
        } else {
            m_spec.scale = scale * oldScale;
        }
    }
    /** Save out preprocess instructions*/
    if (translation == CENTER) {
        m_spec.preprocess.append(ArticulatedModel::Instruction(Any(Any::ARRAY, "moveCenterToOrigin")));
    } else if (translation == BASE) {
        m_spec.preprocess.append(ArticulatedModel::Instruction(Any(Any::ARRAY, "moveBaseToOrigin")));
    }

    /** Save out OBJOptions, only if obj*/
    if (endsWith(toLower(m_spec.filename), ".obj")) {
        ParseOBJ::Options objOptions;
        objOptions.texCoord1Mode   = texCoord1Mode;
        objOptions.stripRefraction = stripRefraction;
        m_spec.objOptions          = objOptions;
    }
}


bool ArticulatedModelSpecificationEditorDialog::getSpecification(ArticulatedModel::Specification& spec, const String& caption) {
    setCaption(caption);
    m_spec = spec;

    showModal(m_osWindow);

    if (ok) {
        finalizeSpecification();
        spec = m_spec;
    }
    return ok;
}

void ArticulatedModelSpecificationEditorDialog::save() {
    const float  oldScale     = m_spec.scale;
    const String oldName      = m_spec.filename;
    finalizeSpecification();
    //Remove path from filename, as we save in the same directory as the original file
    const String location     = FilePath::parent (m_spec.filename);
    const String baseFileName = FilePath::base   (m_spec.filename); 
    const String fullFileName = FilePath::baseExt(m_spec.filename);

    m_spec.filename = fullFileName;
    Any a = m_spec.toAny();
    a.save(location + baseFileName + ".ArticulatedModel.Any");
    m_spec.filename = oldName;
    m_spec.scale    = oldScale; 
}

void ArticulatedModelSpecificationEditorDialog::close() {
    setVisible(false);
    m_manager->remove(dynamic_pointer_cast<Widget>(shared_from_this()));
}


bool ArticulatedModelSpecificationEditorDialog::onEvent(const GEvent& e) {

    const bool handled = GuiWindow::onEvent(e);

    // Check after the (maybe key) event is processed normally
    okButton->setEnabled(true);

    if (handled) {
        return true;
    }

    if ((e.type == GEventType::GUI_ACTION) && e.gui.control == saveButton) {
        save();
    }

    if ((e.type == GEventType::GUI_ACTION) && 
        ((e.gui.control == cancelButton) ||
         (e.gui.control == fileNameBox) ||
         (e.gui.control == okButton) ||
         (e.gui.control == scaleBox))) {
        ok = (e.gui.control != cancelButton);
        close();
        return true;
    } else if ((e.type == GEventType::KEY_DOWN) && (e.key.keysym.sym == GKey::ESCAPE)) {
        // Cancel the dialog
        ok = false;
        close();
        return true;
    }

    return false;
}

} // namespace G3D
