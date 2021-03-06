//==========================================================================
//  AIDA Detector description implementation for LCD
//--------------------------------------------------------------------------
// Copyright (C) Organisation europeenne pour la Recherche nucleaire (CERN)
// All rights reserved.
//
// For the licensing terms see $DD4hepINSTALL/LICENSE.
// For the list of contributors see $DD4hepINSTALL/doc/CREDITS.
//
// Author     : M.Frank
//
//==========================================================================

// Framework includes
#include "DD4hep/AlignmentsPrinter.h"
#include "DD4hep/AlignedVolumePrinter.h"

using std::string;
using namespace DD4hep;
using namespace DD4hep::Alignments;

/// Initializing constructor
AlignedVolumePrinter::AlignedVolumePrinter(const string& pref, int flg)
  : AlignmentsProcessor(0), name("Alignment"), prefix(pref), printLevel(INFO), m_flag(flg)
{
}

/// Initializing constructor
AlignedVolumePrinter::AlignedVolumePrinter(UserPool* p, const std::string& pref,int flg)
  : AlignmentsProcessor(p), name("Alignment"), prefix(pref), printLevel(INFO), m_flag(flg)
{
}

/// Callback to output alignments information
int AlignedVolumePrinter::operator()(Alignment a)    {
  printAlignment(printLevel, name, a);
  return 1;
}

/// Container callback for object processing
int AlignedVolumePrinter::operator()(Container container)   {
  printContainer(printLevel, name, container, m_pool);
  return 1;
}

/// Callback to output alignments information of an entire DetElement
int AlignedVolumePrinter::processElement(DetElement de)  {
  printElementPlacement(printLevel, name, de, m_pool);
  return 1;
}
