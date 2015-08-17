/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "labcurve.h"
#include <iomanip>
#include "../rtengine/improcfun.h"
#include "edit.h"

using namespace rtengine;
using namespace rtengine::procparams;

LCurve::LCurve () : FoldableToolPanel(this, "labcurves", M("TP_LABCURVE_LABEL"))
{

    std::vector<GradientMilestone> milestones;

    brightness = Gtk::manage (new Adjuster (M("TP_LABCURVE_BRIGHTNESS"), -100., 100., 1., 0.));
    contrast   = Gtk::manage (new Adjuster (M("TP_LABCURVE_CONTRAST"), -100., 100., 1., 0.));
    chromaticity   = Gtk::manage (new Adjuster (M("TP_LABCURVE_CHROMATICITY"), -100., 100., 1., 0.));
    chromaticity->set_tooltip_markup(M("TP_LABCURVE_CHROMA_TOOLTIP"));

    pack_start (*brightness);
    brightness->show ();

    pack_start (*contrast);
    contrast->show ();

    pack_start (*chromaticity);
    chromaticity->show ();

    brightness->setAdjusterListener (this);
    contrast->setAdjusterListener (this);
    chromaticity->setAdjusterListener (this);

    //%%%%%%%%%%%%%%%%%%
    Gtk::HSeparator *hsep2 = Gtk::manage (new  Gtk::HSeparator());
    hsep2->show ();
    pack_start (*hsep2, Gtk::PACK_EXPAND_WIDGET, 4);

    avoidcolorshift = Gtk::manage (new Gtk::CheckButton (M("TP_LABCURVE_AVOIDCOLORSHIFT")));
    avoidcolorshift->set_tooltip_text (M("TP_LABCURVE_AVOIDCOLORSHIFT_TOOLTIP"));
    pack_start (*avoidcolorshift, Gtk::PACK_SHRINK, 4);

    lcredsk = Gtk::manage (new Gtk::CheckButton (M("TP_LABCURVE_LCREDSK")));
    lcredsk->set_tooltip_markup (M("TP_LABCURVE_LCREDSK_TIP"));
    pack_start (*lcredsk);

    rstprotection = Gtk::manage ( new Adjuster (M("TP_LABCURVE_RSTPROTECTION"), 0., 100., 0.1, 0.) );
    pack_start (*rstprotection);
    rstprotection->show ();

    rstprotection->setAdjusterListener (this);
    rstprotection->set_tooltip_text (M("TP_LABCURVE_RSTPRO_TOOLTIP"));

    acconn = avoidcolorshift->signal_toggled().connect( sigc::mem_fun(*this, &LCurve::avoidcolorshift_toggled) );
    lcconn = lcredsk->signal_toggled().connect( sigc::mem_fun(*this, &LCurve::lcredsk_toggled) );

    //%%%%%%%%%%%%%%%%%%%

    Gtk::HSeparator *hsep3 = Gtk::manage (new  Gtk::HSeparator());
    hsep3->show ();
    pack_start (*hsep3, Gtk::PACK_EXPAND_WIDGET, 4);

    curveEditorG = new CurveEditorGroup (options.lastLabCurvesDir);
    curveEditorG->setCurveListener (this);

    lshape = static_cast<DiagonalCurveEditor*>(curveEditorG->addCurve(CT_Diagonal, "L*"));
    lshape->setTooltip(M("TP_LABCURVE_CURVEEDITOR_LL_TOOLTIP"));
    lshape->setEditID(EUID_Lab_LCurve, BT_SINGLEPLANE_FLOAT);

    ashape = static_cast<DiagonalCurveEditor*>(curveEditorG->addCurve(CT_Diagonal, "a*"));
    ashape->setEditID(EUID_Lab_aCurve, BT_SINGLEPLANE_FLOAT);

    ashape->setRangeLabels(
        M("TP_LABCURVE_CURVEEDITOR_A_RANGE1"), M("TP_LABCURVE_CURVEEDITOR_A_RANGE2"),
        M("TP_LABCURVE_CURVEEDITOR_A_RANGE3"), M("TP_LABCURVE_CURVEEDITOR_A_RANGE4")
    );
    //from green to magenta
    milestones.clear();
    milestones.push_back( GradientMilestone(0., 0., 1., 0.) );
    milestones.push_back( GradientMilestone(1., 1., 0., 1.) );
    ashape->setBottomBarBgGradient(milestones);
    ashape->setLeftBarBgGradient(milestones);
    milestones.clear();

    bshape = static_cast<DiagonalCurveEditor*>(curveEditorG->addCurve(CT_Diagonal, "b*"));
    bshape->setRangeLabels(
        M("TP_LABCURVE_CURVEEDITOR_B_RANGE1"), M("TP_LABCURVE_CURVEEDITOR_B_RANGE2"),
        M("TP_LABCURVE_CURVEEDITOR_B_RANGE3"), M("TP_LABCURVE_CURVEEDITOR_B_RANGE4")
    );
    bshape->setEditID(EUID_Lab_bCurve, BT_SINGLEPLANE_FLOAT);

    //from blue to yellow
    milestones.clear();
    milestones.push_back( GradientMilestone(0., 0., 0., 1.) );
    milestones.push_back( GradientMilestone(1., 1., 1., 0.) );
    bshape->setBottomBarBgGradient(milestones);
    bshape->setLeftBarBgGradient(milestones);
    milestones.clear();

    curveEditorG->newLine();  //  ------------------------------------------------ second line

    lhshape = static_cast<FlatCurveEditor*>(curveEditorG->addCurve(CT_Flat, M("TP_LABCURVE_CURVEEDITOR_LH")));
    lhshape->setTooltip(M("TP_LABCURVE_CURVEEDITOR_LH_TOOLTIP"));
    lhshape->setCurveColorProvider(this, 4);
    lhshape->setEditID(EUID_Lab_LHCurve, BT_SINGLEPLANE_FLOAT);


    chshape = static_cast<FlatCurveEditor*>(curveEditorG->addCurve(CT_Flat, M("TP_LABCURVE_CURVEEDITOR_CH")));
    chshape->setTooltip(M("TP_LABCURVE_CURVEEDITOR_CH_TOOLTIP"));
    chshape->setCurveColorProvider(this, 1);
    chshape->setEditID(EUID_Lab_CHCurve, BT_SINGLEPLANE_FLOAT);


    hhshape = static_cast<FlatCurveEditor*>(curveEditorG->addCurve(CT_Flat, M("TP_LABCURVE_CURVEEDITOR_HH")));
    hhshape->setTooltip(M("TP_LABCURVE_CURVEEDITOR_HH_TOOLTIP"));
    hhshape->setCurveColorProvider(this, 5);
    hhshape->setEditID(EUID_Lab_HHCurve, BT_SINGLEPLANE_FLOAT);

    curveEditorG->newLine();  //  ------------------------------------------------ 3rd line

    ccshape = static_cast<DiagonalCurveEditor*>(curveEditorG->addCurve(CT_Diagonal, M("TP_LABCURVE_CURVEEDITOR_CC")));
    ccshape->setTooltip(M("TP_LABCURVE_CURVEEDITOR_CC_TOOLTIP"));
    ccshape->setEditID(EUID_Lab_CCurve, BT_SINGLEPLANE_FLOAT);
    ccshape->setRangeLabels(
        M("TP_LABCURVE_CURVEEDITOR_CC_RANGE1"), M("TP_LABCURVE_CURVEEDITOR_CC_RANGE2"),
        M("TP_LABCURVE_CURVEEDITOR_CC_RANGE3"), M("TP_LABCURVE_CURVEEDITOR_CC_RANGE4")
    );

    ccshape->setBottomBarColorProvider(this, 2);
    ccshape->setLeftBarColorProvider(this, 2);
    ccshape->setRangeDefaultMilestones(0.05, 0.2, 0.58);

    lcshape = static_cast<DiagonalCurveEditor*>(curveEditorG->addCurve(CT_Diagonal, M("TP_LABCURVE_CURVEEDITOR_LC")));
    lcshape->setTooltip(M("TP_LABCURVE_CURVEEDITOR_LC_TOOLTIP"));
    lcshape->setEditID(EUID_Lab_LCCurve, BT_SINGLEPLANE_FLOAT);

    // left and bottom bar uses the same caller id because the will display the same content
    lcshape->setBottomBarColorProvider(this, 2);
    lcshape->setRangeLabels(
        M("TP_LABCURVE_CURVEEDITOR_CC_RANGE1"), M("TP_LABCURVE_CURVEEDITOR_CC_RANGE2"),
        M("TP_LABCURVE_CURVEEDITOR_CC_RANGE3"), M("TP_LABCURVE_CURVEEDITOR_CC_RANGE4")
    );
    lcshape->setRangeDefaultMilestones(0.05, 0.2, 0.58);

    clshape = static_cast<DiagonalCurveEditor*>(curveEditorG->addCurve(CT_Diagonal, M("TP_LABCURVE_CURVEEDITOR_CL")));
    clshape->setTooltip(M("TP_LABCURVE_CURVEEDITOR_CL_TOOLTIP"));
    clshape->setEditID(EUID_Lab_CLCurve, BT_SINGLEPLANE_FLOAT);

    clshape->setLeftBarColorProvider(this, 2);
    clshape->setRangeDefaultMilestones(0.25, 0.5, 0.75);
    milestones.push_back( GradientMilestone(0., 0., 0., 0.) );
    milestones.push_back( GradientMilestone(1., 1., 1., 1.) );

    clshape->setBottomBarBgGradient(milestones);


    // Setting the gradient milestones

    // from black to white
    milestones.push_back( GradientMilestone(0., 0., 0., 0.) );
    milestones.push_back( GradientMilestone(1., 1., 1., 1.) );
    lshape->setBottomBarBgGradient(milestones);
    lshape->setLeftBarBgGradient(milestones);
    milestones.push_back( GradientMilestone(0., 0., 0., 0.) );
    milestones.push_back( GradientMilestone(1., 1., 1., 1.) );
    lcshape->setRangeDefaultMilestones(0.05, 0.2, 0.58);

    lcshape->setBottomBarBgGradient(milestones);

    milestones.at(0).r = milestones.at(0).g = milestones.at(0).b = 0.1;
    milestones.at(1).r = milestones.at(1).g = milestones.at(1).b = 0.8;
    lcshape->setLeftBarBgGradient(milestones);

    // whole hue range
    milestones.clear();

    for (int i = 0; i < 7; i++) {
        float R, G, B;
        float x = float(i) * (1.0f / 6.0);
        Color::hsv2rgb01(x, 0.5f, 0.5f, R, G, B);
        milestones.push_back( GradientMilestone(double(x), double(R), double(G), double(B)) );
    }

    chshape->setBottomBarBgGradient(milestones);
    lhshape->setBottomBarBgGradient(milestones);
    hhshape->setBottomBarBgGradient(milestones);


    // This will add the reset button at the end of the curveType buttons
    curveEditorG->curveListComplete();

    pack_start (*curveEditorG, Gtk::PACK_SHRINK, 4);
    Gtk::HSeparator *hsepdh = Gtk::manage (new  Gtk::HSeparator());
    hsepdh->show ();
    pack_start (*hsepdh, Gtk::PACK_EXPAND_WIDGET, 4);

    Gtk::Frame* dehazFrame = Gtk::manage (new Gtk::Frame (M("TP_DEHAZE_LAB")) );
    dehazFrame->set_tooltip_text(M("TP_DEHAZE_LAB_TOOLTIP"));
    dehazFrame->set_border_width(0);
    dehazFrame->set_label_align(0.025, 0.5);

    Gtk::VBox * dehazVBox = Gtk::manage ( new Gtk::VBox());
    dehazVBox->set_border_width(4);
    dehazVBox->set_spacing(2);

    dhbox = Gtk::manage (new Gtk::HBox ());
    labmdh = Gtk::manage (new Gtk::Label (M("TP_DEHAZE_MET") + ":"));
    dhbox->pack_start (*labmdh, Gtk::PACK_SHRINK, 1);

    dehazmet = Gtk::manage (new MyComboBoxText ());
    dehazmet->append_text (M("TP_DEHAZ_NONE"));
    dehazmet->append_text (M("TP_DEHAZ_UNI"));
    dehazmet->append_text (M("TP_DEHAZ_LOW"));
    dehazmet->append_text (M("TP_DEHAZ_HIGH"));
    dehazmet->set_active(0);
    dehazmetConn = dehazmet->signal_changed().connect ( sigc::mem_fun(*this, &LCurve::dehazmetChanged) );
  //  dehazmet->set_tooltip_markup (M("TP_DEHAZ_MET_TOOLTIP"));
    dhbox->pack_start(*dehazmet);
    dehazVBox->pack_start(*dhbox);

    str = Gtk::manage (new Adjuster (M("TP_LABCURVE_STR"), 0, 100., 1., 70.));
    scal   = Gtk::manage (new Adjuster (M("TP_LABCURVE_SCAL"), 1, 8., 1., 3.));
    neigh = Gtk::manage (new Adjuster (M("TP_LABCURVE_NEIGH"), 6, 100., 1., 80.));
    gain   = Gtk::manage (new Adjuster (M("TP_LABCURVE_GAIN"), 0.9, 1.1, 0.01, 1.));
    offs   = Gtk::manage (new Adjuster (M("TP_LABCURVE_OFFS"), -50, 50, 1, 0));
    dehazVBox->pack_start (*str);
    str->show ();

    dehazVBox->pack_start (*scal);
    scal->show ();

    dehazVBox->pack_start (*neigh);
    neigh->show ();

 //   dehazVBox->pack_start (*gain);
 //   gain->show ();

 //   dehazVBox->pack_start (*offs);
 //   offs->show ();

    str->setAdjusterListener (this);
    scal->setAdjusterListener (this);
    neigh->setAdjusterListener (this);
    gain->setAdjusterListener (this);
    offs->setAdjusterListener (this);
    dehazFrame->add(*dehazVBox);
    pack_start (*dehazFrame);

}

LCurve::~LCurve ()
{
    delete curveEditorG;
}

void LCurve::read (const ProcParams* pp, const ParamsEdited* pedited)
{

    disableListener ();
    dehazmetConn.block(true);

    if (pedited) {
        brightness->setEditedState (pedited->labCurve.brightness ? Edited : UnEdited);
        contrast->setEditedState (pedited->labCurve.contrast ? Edited : UnEdited);
       chromaticity->setEditedState (pedited->labCurve.chromaticity ? Edited : UnEdited);

        //%%%%%%%%%%%%%%%%%%%%%%
        rstprotection->setEditedState (pedited->labCurve.rstprotection ? Edited : UnEdited);
        avoidcolorshift->set_inconsistent (!pedited->labCurve.avoidcolorshift);
        lcredsk->set_inconsistent (!pedited->labCurve.lcredsk);
        str->setEditedState (pedited->labCurve.str ? Edited : UnEdited);
        scal->setEditedState (pedited->labCurve.scal ? Edited : UnEdited);
        neigh->setEditedState (pedited->labCurve.neigh ? Edited : UnEdited);
        gain->setEditedState (pedited->labCurve.gain ? Edited : UnEdited);
        offs->setEditedState (pedited->labCurve.offs ? Edited : UnEdited);

        if (!pedited->labCurve.dehazmet) {
            dehazmet->set_active (3);
        }

        //%%%%%%%%%%%%%%%%%%%%%%

        lshape->setUnChanged   (!pedited->labCurve.lcurve);
        ashape->setUnChanged   (!pedited->labCurve.acurve);
        bshape->setUnChanged   (!pedited->labCurve.bcurve);
        ccshape->setUnChanged  (!pedited->labCurve.cccurve);
        chshape->setUnChanged  (!pedited->labCurve.chcurve);
        lhshape->setUnChanged  (!pedited->labCurve.lhcurve);
        hhshape->setUnChanged  (!pedited->labCurve.hhcurve);
        lcshape->setUnChanged  (!pedited->labCurve.lccurve);
        clshape->setUnChanged  (!pedited->labCurve.clcurve);
    }

    brightness->setValue    (pp->labCurve.brightness);
    contrast->setValue      (pp->labCurve.contrast);
    chromaticity->setValue  (pp->labCurve.chromaticity);
    adjusterChanged(chromaticity, pp->labCurve.chromaticity); // To update the GUI sensitiveness
    neigh->setValue    (pp->labCurve.neigh);
    gain->setValue      (pp->labCurve.gain);
    offs->setValue  (pp->labCurve.offs);
    str->setValue    (pp->labCurve.str);
    scal->setValue      (pp->labCurve.scal);
    
    dehazmet->set_active (0);
    if (pp->labCurve.dehazmet == "none") {
        dehazmet->set_active (0);
    } else if (pp->labCurve.dehazmet == "uni") {
        dehazmet->set_active (1);
    } else if (pp->labCurve.dehazmet == "low") {
        dehazmet->set_active (2);
    } else if (pp->labCurve.dehazmet == "high") {
        dehazmet->set_active (3);
    }
    dehazmetChanged ();

    //%%%%%%%%%%%%%%%%%%%%%%
    rstprotection->setValue (pp->labCurve.rstprotection);

    bwtconn.block (true);
    acconn.block (true);
    lcconn.block (true);
    avoidcolorshift->set_active (pp->labCurve.avoidcolorshift);
    lcredsk->set_active (pp->labCurve.lcredsk);

    bwtconn.block (false);
    acconn.block (false);
    lcconn.block (false);

    lastACVal = pp->labCurve.avoidcolorshift;
    lastLCVal = pp->labCurve.lcredsk;
    //%%%%%%%%%%%%%%%%%%%%%%

    lshape->setCurve   (pp->labCurve.lcurve);
    ashape->setCurve   (pp->labCurve.acurve);
    bshape->setCurve   (pp->labCurve.bcurve);
    ccshape->setCurve  (pp->labCurve.cccurve);
    chshape->setCurve  (pp->labCurve.chcurve);
    lhshape->setCurve  (pp->labCurve.lhcurve);
    hhshape->setCurve  (pp->labCurve.hhcurve);
    lcshape->setCurve  (pp->labCurve.lccurve);
    clshape->setCurve  (pp->labCurve.clcurve);

    queue_draw();
    dehazmetConn.block(false);

    enableListener ();
}

void LCurve::autoOpenCurve ()
{
    // Open up the first curve if selected
    bool active = lshape->openIfNonlinear();

    if (!active) {
        ashape->openIfNonlinear();
    }

    if (!active) {
        bshape->openIfNonlinear();
    }

    if (!active) {
        ccshape->openIfNonlinear();
    }

    if (!active) {
        chshape->openIfNonlinear();
    }

    if (!active) {
        lhshape->openIfNonlinear();
    }

    if (!active) {
        hhshape->openIfNonlinear();
    }

    if (!active) {
        lcshape->openIfNonlinear();
    }

    if (!active) {
        clshape->openIfNonlinear();
    }
}

void LCurve::setEditProvider  (EditDataProvider *provider)
{
    lshape->setEditProvider(provider);
    ccshape->setEditProvider(provider);
    lcshape->setEditProvider(provider);
    clshape->setEditProvider(provider);
    lhshape->setEditProvider(provider);
    chshape->setEditProvider(provider);
    hhshape->setEditProvider(provider);
    ashape->setEditProvider(provider);
    bshape->setEditProvider(provider);

}


void LCurve::write (ProcParams* pp, ParamsEdited* pedited)
{

    pp->labCurve.brightness    = brightness->getValue ();
    pp->labCurve.contrast      = (int)contrast->getValue ();
    pp->labCurve.chromaticity  = (int)chromaticity->getValue ();
    pp->labCurve.str    = str->getValue ();
    pp->labCurve.scal      = (int)scal->getValue ();
    pp->labCurve.neigh    = neigh->getValue ();
    pp->labCurve.gain      = (int)gain->getValue ();
    pp->labCurve.offs  = (int)offs->getValue ();

    //%%%%%%%%%%%%%%%%%%%%%%
    pp->labCurve.avoidcolorshift = avoidcolorshift->get_active ();
    pp->labCurve.lcredsk         = lcredsk->get_active ();

    pp->labCurve.rstprotection   = rstprotection->getValue ();
    //%%%%%%%%%%%%%%%%%%%%%%

    pp->labCurve.lcurve  = lshape->getCurve ();
    pp->labCurve.acurve  = ashape->getCurve ();
    pp->labCurve.bcurve  = bshape->getCurve ();
    pp->labCurve.cccurve = ccshape->getCurve ();
    pp->labCurve.chcurve = chshape->getCurve ();
    pp->labCurve.lhcurve = lhshape->getCurve ();
    pp->labCurve.hhcurve = hhshape->getCurve ();
    pp->labCurve.lccurve = lcshape->getCurve ();
    pp->labCurve.clcurve = clshape->getCurve ();

    if (pedited) {
        pedited->labCurve.brightness   = brightness->getEditedState ();
        pedited->labCurve.contrast     = contrast->getEditedState ();
        pedited->labCurve.chromaticity = chromaticity->getEditedState ();

        //%%%%%%%%%%%%%%%%%%%%%%
        pedited->labCurve.avoidcolorshift = !avoidcolorshift->get_inconsistent();
        pedited->labCurve.lcredsk         = !lcredsk->get_inconsistent();

        pedited->labCurve.rstprotection   = rstprotection->getEditedState ();
        pedited->labCurve.dehazmet  = dehazmet->get_active_row_number() != 3;
       
        //%%%%%%%%%%%%%%%%%%%%%%
        pedited->labCurve.str   = str->getEditedState ();
        pedited->labCurve.scal     = scal->getEditedState ();
        pedited->labCurve.neigh   = neigh->getEditedState ();
        pedited->labCurve.gain     = gain->getEditedState ();
        pedited->labCurve.offs = offs->getEditedState ();


        pedited->labCurve.lcurve    = !lshape->isUnChanged ();
        pedited->labCurve.acurve    = !ashape->isUnChanged ();
        pedited->labCurve.bcurve    = !bshape->isUnChanged ();
        pedited->labCurve.cccurve   = !ccshape->isUnChanged ();
        pedited->labCurve.chcurve   = !chshape->isUnChanged ();
        pedited->labCurve.lhcurve   = !lhshape->isUnChanged ();
        pedited->labCurve.hhcurve   = !hhshape->isUnChanged ();
        pedited->labCurve.lccurve   = !lcshape->isUnChanged ();
        pedited->labCurve.clcurve   = !clshape->isUnChanged ();
    if (dehazmet->get_active_row_number() == 0) {
        pp->labCurve.dehazmet = "none";
    } else if (dehazmet->get_active_row_number() == 1) {
        pp->labCurve.dehazmet = "uni";
    } else if (dehazmet->get_active_row_number() == 2) {
        pp->labCurve.dehazmet = "low";
    } else if (dehazmet->get_active_row_number() == 3) {
        pp->labCurve.dehazmet = "high";
    }
        
        
    }
}

void LCurve::dehazmetChanged()
{
    if (listener) { 
        listener->panelChanged (Evdehazmet, dehazmet->get_active_text ());
    }
 }


void LCurve::setDefaults (const ProcParams* defParams, const ParamsEdited* pedited)
{

    brightness->setDefault (defParams->labCurve.brightness);
    contrast->setDefault (defParams->labCurve.contrast);
    chromaticity->setDefault (defParams->labCurve.chromaticity);
    rstprotection->setDefault (defParams->labCurve.rstprotection);
    neigh->setDefault (defParams->labCurve.neigh);
    gain->setDefault (defParams->labCurve.gain);
    offs->setDefault (defParams->labCurve.offs);
    str->setDefault (defParams->labCurve.str);
    scal->setDefault (defParams->labCurve.scal);

    if (pedited) {
        brightness->setDefaultEditedState (pedited->labCurve.brightness ? Edited : UnEdited);
        contrast->setDefaultEditedState (pedited->labCurve.contrast ? Edited : UnEdited);
        chromaticity->setDefaultEditedState (pedited->labCurve.chromaticity ? Edited : UnEdited);
        rstprotection->setDefaultEditedState (pedited->labCurve.rstprotection ? Edited : UnEdited);
       neigh->setDefaultEditedState (pedited->labCurve.neigh ? Edited : UnEdited);
        gain->setDefaultEditedState (pedited->labCurve.gain ? Edited : UnEdited);
        offs->setDefaultEditedState (pedited->labCurve.offs ? Edited : UnEdited);
        str->setDefaultEditedState (pedited->labCurve.str ? Edited : UnEdited);
        scal->setDefaultEditedState (pedited->labCurve.scal ? Edited : UnEdited);

    } else {
        brightness->setDefaultEditedState (Irrelevant);
        contrast->setDefaultEditedState (Irrelevant);
        chromaticity->setDefaultEditedState (Irrelevant);
        rstprotection->setDefaultEditedState (Irrelevant);
        neigh->setDefaultEditedState (Irrelevant);
        gain->setDefaultEditedState (Irrelevant);
        offs->setDefaultEditedState (Irrelevant);
        str->setDefaultEditedState (Irrelevant);
        scal->setDefaultEditedState (Irrelevant);
    }
}

//%%%%%%%%%%%%%%%%%%%%%%
//Color shift control changed
void LCurve::avoidcolorshift_toggled ()
{

    if (batchMode) {
        if (avoidcolorshift->get_inconsistent()) {
            avoidcolorshift->set_inconsistent (false);
            acconn.block (true);
            avoidcolorshift->set_active (false);
            acconn.block (false);
        } else if (lastACVal) {
            avoidcolorshift->set_inconsistent (true);
        }

        lastACVal = avoidcolorshift->get_active ();
    }

    if (listener) {
        if (avoidcolorshift->get_active ()) {
            listener->panelChanged (EvLAvoidColorShift, M("GENERAL_ENABLED"));
        } else {
            listener->panelChanged (EvLAvoidColorShift, M("GENERAL_DISABLED"));
        }
    }
}

void LCurve::lcredsk_toggled ()
{

    if (batchMode) {
        if (lcredsk->get_inconsistent()) {
            lcredsk->set_inconsistent (false);
            lcconn.block (true);
            lcredsk->set_active (false);
            lcconn.block (false);
        } else if (lastLCVal) {
            lcredsk->set_inconsistent (true);
        }

        lastLCVal = lcredsk->get_active ();
    } else {
        lcshape->refresh();
    }

    if (listener) {
        if (lcredsk->get_active ()) {
            listener->panelChanged (EvLLCredsk, M("GENERAL_ENABLED"));
        } else {
            listener->panelChanged (EvLLCredsk, M("GENERAL_DISABLED"));
        }
    }
}

//%%%%%%%%%%%%%%%%%%%%%%

/*
 * Curve listener
 *
 * If more than one curve has been added, the curve listener is automatically
 * set to 'multi=true', and send a pointer of the modified curve in a parameter
 */
void LCurve::curveChanged (CurveEditor* ce)
{

    if (listener) {
        if (ce == lshape) {
            listener->panelChanged (EvLLCurve, M("HISTORY_CUSTOMCURVE"));
        }

        if (ce == ashape) {
            listener->panelChanged (EvLaCurve, M("HISTORY_CUSTOMCURVE"));
        }

        if (ce == bshape) {
            listener->panelChanged (EvLbCurve, M("HISTORY_CUSTOMCURVE"));
        }

        if (ce == ccshape) {
            listener->panelChanged (EvLCCCurve, M("HISTORY_CUSTOMCURVE"));
        }

        if (ce == chshape) {
            listener->panelChanged (EvLCHCurve, M("HISTORY_CUSTOMCURVE"));
        }

        if (ce == lhshape) {
            listener->panelChanged (EvLLHCurve, M("HISTORY_CUSTOMCURVE"));
        }

        if (ce == hhshape) {
            listener->panelChanged (EvLHHCurve, M("HISTORY_CUSTOMCURVE"));
        }

        if (ce == lcshape) {
            listener->panelChanged (EvLLCCurve, M("HISTORY_CUSTOMCURVE"));
        }

        if (ce == clshape) {
            listener->panelChanged (EvLCLCurve, M("HISTORY_CUSTOMCURVE"));
        }
    }
}

void LCurve::adjusterChanged (Adjuster* a, double newval)
{

    Glib::ustring costr;

    if (a == brightness) {
        costr = Glib::ustring::format (std::setw(3), std::fixed, std::setprecision(2), a->getValue());
    } else if (a == rstprotection) {
        costr = Glib::ustring::format (std::setw(3), std::fixed, std::setprecision(1), a->getValue());
    } else {
        costr = Glib::ustring::format ((int)a->getValue());
    }

    if (a == brightness) {
        if (listener) {
            listener->panelChanged (EvLBrightness, costr);
        }
    } else if (a == contrast) {
        if (listener) {
            listener->panelChanged (EvLContrast, costr);
        }
    } else if (a == rstprotection) {
        if (listener) {
            listener->panelChanged (EvLRSTProtection, costr);
        }
    } else if (a == neigh) {
        if (listener) {
            listener->panelChanged (EvLneigh, costr);
        }
    } else if (a == str) {
        if (listener) {
            listener->panelChanged (EvLstr, costr);
        }
    } else if (a == scal) {
        if (listener) {
            listener->panelChanged (EvLscal, costr);
        }

    } else if (a == gain) {
        if (listener) {
            listener->panelChanged (EvLgain, costr);
        }
    } else if (a == offs) {
        if (listener) {
            listener->panelChanged (EvLoffs, costr);
        }
       
    } else if (a == chromaticity) {
        if (multiImage) {
            //if chromaticity==-100 (lowest value), we enter the B&W mode and avoid color shift and rstprotection has no effect
            rstprotection->set_sensitive( true );
            avoidcolorshift->set_sensitive( true );
            lcredsk->set_sensitive( true );
        } else {
            //if chromaticity==-100 (lowest value), we enter the B&W mode and avoid color shift and rstprotection has no effect
            rstprotection->set_sensitive( int(newval) > -100 ); //no reason for grey rstprotection
            avoidcolorshift->set_sensitive( int(newval) > -100 );
            lcredsk->set_sensitive( int(newval) > -100 );
        }

        if (listener) {
            listener->panelChanged (EvLSaturation, costr);
        }
    }
}

void LCurve::colorForValue (double valX, double valY, enum ColorCaller::ElemType elemType, int callerId, ColorCaller *caller)
{

    float R, G, B;

    if (elemType == ColorCaller::CCET_VERTICAL_BAR) {
        valY = 0.5;
    }

    if (callerId == 1) {         // ch - main curve

        Color::hsv2rgb01(float(valX), float(valY), 0.5f, R, G, B);
    } else if (callerId == 2) {  // cc - bottom bar

        float value = (1.f - 0.7f) * float(valX) + 0.7f;
        // whole hue range
        // Y axis / from 0.15 up to 0.75 (arbitrary values; was 0.45 before)
        Color::hsv2rgb01(float(valY), float(valX), value, R, G, B);
    } else if (callerId == 3) {  // lc - bottom bar

        float value = (1.f - 0.7f) * float(valX) + 0.7f;

        if (lcredsk->get_active()) {
            // skin range
            // -0.1 rad < Hue < 1.6 rad
            // Y axis / from 0.92 up to 0.14056
            float hue = (1.14056f - 0.92f) * float(valY) + 0.92f;

            if (hue > 1.0f) {
                hue -= 1.0f;
            }

            // Y axis / from 0.15 up to 0.75 (arbitrary values; was 0.45 before)
            Color::hsv2rgb01(hue, float(valX), value, R, G, B);
        } else {
            // whole hue range
            // Y axis / from 0.15 up to 0.75 (arbitrary values; was 0.45 before)
            Color::hsv2rgb01(float(valY), float(valX), value, R, G, B);
        }
    } else if (callerId == 4) {  // LH - bottom bar
        Color::hsv2rgb01(float(valX), 0.5f, float(valY), R, G, B);
    } else if (callerId == 5) {  // HH - bottom bar
        float h = float((valY - 0.5) * 0.3 + valX);

        if (h > 1.0f) {
            h -= 1.0f;
        } else if (h < 0.0f) {
            h += 1.0f;
        }

        Color::hsv2rgb01(h, 0.5f, 0.5f, R, G, B);
    }

    caller->ccRed = double(R);
    caller->ccGreen = double(G);
    caller->ccBlue = double(B);
}

void LCurve::setBatchMode (bool batchMode)
{

    ToolPanel::setBatchMode (batchMode);
    brightness->showEditedCB ();
    contrast->showEditedCB ();
    chromaticity->showEditedCB ();
    rstprotection->showEditedCB ();
    dehazmet->append_text (M("GENERAL_UNCHANGED"));
    neigh->showEditedCB ();
    gain->showEditedCB ();
    offs->showEditedCB ();
    str->showEditedCB ();
    scal->showEditedCB ();

    curveEditorG->setBatchMode (batchMode);
    lcshape->setBottomBarColorProvider(NULL, -1);
    lcshape->setLeftBarColorProvider(NULL, -1);
}


void LCurve::updateCurveBackgroundHistogram (LUTu & histToneCurve, LUTu & histLCurve, LUTu & histCCurve,/* LUTu & histCLurve, LUTu & histLLCurve,*/ LUTu & histLCAM,  LUTu & histCCAM, LUTu & histRed, LUTu & histGreen, LUTu & histBlue, LUTu & histLuma)
{

    lshape->updateBackgroundHistogram (histLCurve);
    ccshape->updateBackgroundHistogram (histCCurve);
//  clshape->updateBackgroundHistogram (histCLurve);
//  lcshape->updateBackgroundHistogram (histLLCurve);

}

void LCurve::setAdjusterBehavior (bool bradd, bool contradd, bool satadd)
{

    brightness->setAddMode(bradd);
    contrast->setAddMode(contradd);
    chromaticity->setAddMode(satadd);
}

void LCurve::trimValues (rtengine::procparams::ProcParams* pp)
{

    brightness->trimValue(pp->labCurve.brightness);
    contrast->trimValue(pp->labCurve.contrast);
    chromaticity->trimValue(pp->labCurve.chromaticity);
    str->trimValue(pp->labCurve.str);
    scal->trimValue(pp->labCurve.scal);
    neigh->trimValue(pp->labCurve.neigh);
    gain->trimValue(pp->labCurve.gain);
    offs->trimValue(pp->labCurve.offs);
}
