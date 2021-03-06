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
#ifndef DD4HEP_DDCORE_CONDITIONSPRINTER_H
#define DD4HEP_DDCORE_CONDITIONSPRINTER_H

// Framework includes
#include "DD4hep/Printout.h"
#include "DD4hep/ConditionsProcessor.h"

/// Namespace for the AIDA detector description toolkit
namespace DD4hep {

  /// Namespace for the AIDA detector description toolkit supporting XML utilities
  namespace Conditions {

    /// Generic Conditions data dumper.
    /**
     *   Please note that the principle of locality applies:
     *   The object is designed for stack allocation and configuration.
     *   It may NOT be shared across threads!
     *
     *   \author  M.Frank
     *   \version 1.0
     *   \date    31/03/2016
     *   \ingroup DD4HEP_DDDB
     */
    class ConditionsPrinter : public ConditionsProcessor  {
    public:
      /// Printer name. Want to know who is printing what
      std::string   name;
      /// Printout prefix
      std::string   prefix;
      /// Printout level
      PrintLevel    printLevel;

    protected:
      /// Printout processing and customization flag
      int           m_flag;

    public:
      /// Initializing constructor
      ConditionsPrinter(UserPool* pool, const std::string& prefix="", 
                        int flag=Condition::NO_NAME|Condition::WITH_IOV|Condition::WITH_ADDRESS);
      /// Initializing constructor
      ConditionsPrinter(const std::string& prefix="", 
                        int flag=Condition::NO_NAME|Condition::WITH_IOV|Condition::WITH_ADDRESS);
      /// Default destructor
      virtual ~ConditionsPrinter() = default;
      /// Set name for printouts
      void setName(const std::string& value)    {  name = value;   }
      /// Set prefix for printouts
      void setPrefix(const std::string& value)  {  prefix = value; }
      /// Callback to output conditions information
      virtual int operator()(Condition cond)  override;
      /// Container callback for object processing
      virtual int operator()(Container container)  override;
    };

  }    /* End namespace Conditions           */
}      /* End namespace DD4hep               */
#endif /* DD4HEP_DDCORE_CONDITIONSPRINTER_H  */
