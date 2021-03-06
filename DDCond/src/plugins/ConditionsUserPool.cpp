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
#ifndef DDCOND_CONDITIONSMAPPEDUSERPOOL_H
#define DDCOND_CONDITIONSMAPPEDUSERPOOL_H

// Framework include files
#include "DDCond/ConditionsPool.h"

// C/C++ include files
#include <map>
#include <unordered_map>

/// Namespace for the AIDA detector description toolkit
namespace DD4hep {

  /// Namespace for the geometry part of the AIDA detector description toolkit
  namespace Conditions {

    /// Forward declarations
    class ConditionsDataLoader;
    
    /// Class implementing the conditions user pool for a given IOV type
    /**
     *
     *  Please note:
     *  Users should not directly interact with object instances of this type.
     *  Data are not thread protected and interaction may cause serious harm.
     *  Only the ConditionsManager implementation should interact with
     *  this class or any subclass to ensure data integrity.
     *
     *  \author  M.Frank
     *  \version 1.0
     *  \ingroup DD4HEP_CONDITIONS
     */
    template<typename MAPPING> 
    class ConditionsMappedUserPool : public UserPool    {
      typedef MAPPING Mapping;
      Mapping               m_conditions;
      /// IOV Pool as data source
      ConditionsIOVPool*    m_iovPool = 0;
      /// The loader to access non-existing conditions
      ConditionsDataLoader* m_loader = 0;

      /// Internal helper to find conditions
      Condition::Object* i_findCondition(key_type key)  const;

      /// Internal insertion helper
      bool i_insert(Condition::Object* o);

    public:
      /// Default constructor
      ConditionsMappedUserPool(ConditionsManager mgr, ConditionsIOVPool* pool);
      /// Default destructor
      virtual ~ConditionsMappedUserPool();
      /// Print pool content
      virtual void print(const std::string& opt)   const;
      /// Total entry count
      virtual size_t size()  const;
      /// Full cleanup of all managed conditions.
      virtual void clear();
      /// Check a condition for existence
      virtual bool exists(key_type hash)  const;
      /// Check a condition for existence
      virtual bool exists(const ConditionKey& key)  const;
      /// Check if a condition exists in the pool and return it to the caller
      virtual Condition get(key_type hash)  const;
      /// Check if a condition exists in the pool and return it to the caller     
      virtual Condition get(const ConditionKey& key)  const;
      /// Remove condition by key from pool.
      virtual bool remove(key_type hash_key);
      /// Remove condition by key from pool.
      virtual bool remove(const ConditionKey& key);
      /// Register a new condition to this pool
      virtual bool insert(Condition cond);
      /// Prepare user pool for usage (load, fill etc.) according to required IOV
      virtual Result prepare(const IOV&              required,
                             ConditionsSlice&        slice,
                             void*                   user_param)  {
        return prepare_VSN_1(required, slice, user_param);
      }
      /// Prepare user pool for usage (load, fill etc.) according to required IOV
      Result prepare_VSN_1(const IOV&              required,
                           ConditionsSlice&        slice,
                           void*                   user_param);
      /// Evaluate and register all derived conditions from the dependency list
      virtual size_t compute(const Dependencies&     dependencies,
                             void*                   user_param);
    };
  }    /* End namespace Conditions               */
}      /* End namespace DD4hep                   */
#endif /* DDCOND_CONDITIONSMAPPEDUSERPOOL_H      */

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

// Framework include files
//#include "DDCond/ConditionsMappedPool.h"
#include "DD4hep/Printout.h"
#include "DD4hep/Factories.h"
#include "DD4hep/InstanceCount.h"

#include "DDCond/ConditionsIOVPool.h"
#include "DDCond/ConditionsSelectors.h"
#include "DDCond/ConditionsDataLoader.h"
#include "DDCond/ConditionsManagerObject.h"
#include "DDCond/ConditionsDependencyHandler.h"

#include <mutex>

using namespace std;
using namespace DD4hep;
using namespace DD4hep::Conditions;

namespace {

  template <typename T> struct MapSelector : public ConditionsSelect {
    T& m;
    MapSelector(T& o) : m(o) {}
    bool operator()(Condition::Object* o)  const
    { return m.insert(make_pair(o->hash,o)).second;    }
  };
  template <typename T> MapSelector<T> mapSelector(T& container)
  {  return MapSelector<T>(container);   }

  template <typename T> struct Inserter {
    T& m;
    IOV* iov;
    Inserter(T& o, IOV* i=0) : m(o), iov(i) {}
    void operator()(const Condition& c)  {
      Condition::Object* o = c.ptr();
      m.insert(make_pair(o->hash,o));
      if ( iov ) iov->iov_intersection(o->iov->key());
    }
    void operator()(const pair<Condition::key_type,Condition>& e) { (*this)(e.second);  }
  };
}

/// Default constructor
template<typename MAPPING>
ConditionsMappedUserPool<MAPPING>::ConditionsMappedUserPool(ConditionsManager mgr, ConditionsIOVPool* pool) 
  : UserPool(mgr), m_iovPool(pool)
{
  InstanceCount::increment(this);
  if ( mgr.isValid() )  {
    if ( m_iovPool )  {
      m_iov.iovType = m_iovPool->type;
      m_loader = mgr->loader();
      if ( m_loader ) return;
      except("UserPool","The conditions manager is not properly setup. No conditions loader present.");
    }
    except("UserPool","FAILED to create mapped user pool. [Invalid IOV pool]");
  }
  except("UserPool","FAILED to create mapped user pool. [Invalid conditions manager]");
}

/// Default destructor
template<typename MAPPING>
ConditionsMappedUserPool<MAPPING>::~ConditionsMappedUserPool()  {
  clear();
  InstanceCount::decrement(this);
}

template<typename MAPPING> inline Condition::Object* 
ConditionsMappedUserPool<MAPPING>::i_findCondition(key_type key)  const {
  typename MAPPING::const_iterator i=m_conditions.find(key);
#if 0
  if ( i == m_conditions.end() )  {
    print("*"); // This causes CTEST to bail out, due too much output!
  }
#endif
  return i != m_conditions.end() ? (*i).second : 0;
}

template<typename MAPPING> inline bool
ConditionsMappedUserPool<MAPPING>::i_insert(Condition::Object* o)   {
  return m_conditions.insert(make_pair(o->hash,o)).second;
}

/// Total entry count
template<typename MAPPING>
size_t ConditionsMappedUserPool<MAPPING>::size()  const  {
  return  m_conditions.size();
}

/// Print pool content
template<typename MAPPING>
void ConditionsMappedUserPool<MAPPING>::print(const std::string& opt)   const  {
  const IOV* iov = &m_iov;
  printout(INFO,"UserPool","+++ %s Conditions for USER pool with IOV: %-32s [%4d entries]",
           opt.c_str(), iov->str().c_str(), size());
  if ( opt == "*" ) {
    for( const auto& i : m_conditions )   {
      Condition c = i.second;
      printout(INFO,"UserPool","++ %16llX/%16llX Val:%s %s",i.first, c->hash, c->value.c_str(), c.str().c_str());
    }
  }
}

/// Full cleanup of all managed conditions.
template<typename MAPPING>
void ConditionsMappedUserPool<MAPPING>::clear()   {
  m_iov = IOV(0);
  m_conditions.clear();
}

/// Check a condition for existence
template<typename MAPPING>
bool ConditionsMappedUserPool<MAPPING>::exists(key_type hash)  const   {
  return i_findCondition(hash) != 0;
}

/// Check a condition for existence
template<typename MAPPING>
bool ConditionsMappedUserPool<MAPPING>::exists(const ConditionKey& key)  const   {
  return i_findCondition(key.hash) != 0;
}

/// Check if a condition exists in the pool and return it to the caller
template<typename MAPPING>
Condition ConditionsMappedUserPool<MAPPING>::get(key_type hash)  const   {
  return i_findCondition(hash);
}

/// Check if a condition exists in the pool and return it to the caller     
template<typename MAPPING>
Condition ConditionsMappedUserPool<MAPPING>::get(const ConditionKey& key)  const   {
  return i_findCondition(key.hash);
}

/// Register a new condition to this pool
template<typename MAPPING>
bool ConditionsMappedUserPool<MAPPING>::insert(Condition cond)   {
  bool result = i_insert(cond.ptr());
  //printout(INFO,"UserPool","++ INSERT: %16llX Name:%s",cond->hash, cond.name());
  if ( result ) return true;
  except("UserPool","++ Attempt to double insert condition: %s", cond.name());
  return false;
}

/// Remove condition by key from pool.
template<typename MAPPING>
bool ConditionsMappedUserPool<MAPPING>::remove(const ConditionKey& key)   {
  return remove(key.hash);
}

/// Remove condition by key from pool.
template<typename MAPPING>
bool ConditionsMappedUserPool<MAPPING>::remove(key_type hash_key)    {
  typename MAPPING::iterator i = m_conditions.find(hash_key);
  if ( i != m_conditions.end() ) {
    m_conditions.erase(i);
    return true;
  }
  return false;
}

/// Evaluate and register all derived conditions from the dependency list
template<typename MAPPING>
size_t ConditionsMappedUserPool<MAPPING>::compute(const Dependencies& deps,
                                                  void* user_param)
{
  size_t num_updates = 0;
  if ( !deps.empty() )  {
    vector<const ConditionDependency*> missing;
    // Loop over the dependencies and check if they have to be upgraded
    missing.reserve(deps.size());
    for ( const auto& i : deps )  {
      typename MAPPING::iterator j = m_conditions.find(i.first);
      if ( j != m_conditions.end() )  {
        Condition::Object* c = (*j).second;
        // Remeber: key ist first, test is second!
        if ( IOV::key_is_contained(m_iov.keyData,c->iov->keyData) )  {
          /// This condition is no longer valid. remove it!
          /// This condition will be added again by the handler.
          m_conditions.erase(j);
          missing.push_back(i.second.get());
        }
        continue;
      }
      missing.push_back(i.second.get());      
    }
    if ( !missing.empty() )  {
      ConditionsManagerObject*    m(m_manager.access());
      ConditionsDependencyHandler h(m, *this, deps, user_param);
      for ( const ConditionDependency* d : missing )  {
        Condition::Object* c = h(d);
        if ( c ) ++num_updates;
      }
    }
  }
  return num_updates;
}

namespace {
  struct COMP {
    typedef pair<Condition::key_type,ConditionsDescriptor*> Slice;
    typedef pair<Condition::key_type,Condition>   Cond;
    bool operator()(const Slice& a,const Cond& b) const { return a.first < b.first; }
    bool operator()(const Cond& a,const Slice& b) const { return a.first < b.first; }
  };
  pair<Condition::key_type,ConditionDependency*> _to_dep(pair<Condition::key_type,ConditionsDescriptor*>& e)
  { return make_pair(e.second->key.hash,e.second->dependency); }
}

template<typename MAPPING> UserPool::Result
ConditionsMappedUserPool<MAPPING>::prepare_VSN_1(const IOV&              required, 
                                                 ConditionsSlice&        slice,
                                                 void*                   user_param)
{
  typedef std::vector<pair<Condition::key_type,ConditionsDescriptor*> > _Missing;
  const auto& slice_cond = slice.conditions();
  const auto& slice_calc = slice.derived();
  IOV pool_iov(required.iovType);
  Result result;

  // This is a critical operation, because we have to ensure the
  // IOV pools are ONLY manipulated by the current thread.
  // Otherwise the selection and the population are unsafe!
  static mutex lock;
  lock_guard<mutex> guard(lock);

  m_conditions.clear();
  pool_iov.reset().invert();
  m_iovPool->select(required, Operators::mapConditionsSelect(m_conditions), pool_iov);
  m_iov = pool_iov;
  _Missing cond_missing(slice_cond.size()+m_conditions.size());
  _Missing calc_missing(slice_calc.size()+m_conditions.size());

  _Missing::iterator last_cond = set_difference(begin(slice_cond),   end(slice_cond),
                                                begin(m_conditions), end(m_conditions),
                                                begin(cond_missing), COMP());
  int num_cond_miss = int(last_cond-begin(cond_missing));
  printout(num_cond_miss==0 ? DEBUG : INFO,"UserPool",
           "Found %ld missing conditions out of %ld conditions.",
           num_cond_miss, m_conditions.size());
  _Missing::iterator last_calc = set_difference(begin(slice_calc),   end(slice_calc),
                                                begin(m_conditions), end(m_conditions),
                                                begin(calc_missing), COMP());
  int num_calc_miss = int(last_calc-begin(calc_missing));
  printout(num_cond_miss==0 ? DEBUG : INFO,"UserPool",
           "Found %ld missing derived conditions out of %ld conditions.",
           num_calc_miss, m_conditions.size());

  result.loaded   = 0;
  result.computed = 0;
  result.selected = m_conditions.size();
  result.missing  = num_cond_miss+num_calc_miss;
  //
  // Now we load the missing conditions from the conditions loader
  //
  if ( num_cond_miss > 0 )  {
    ConditionsDataLoader::LoadedItems loaded;
    size_t updates = m_loader->load_many(required, cond_missing, loaded, pool_iov);
    if ( updates > 0 )  {
      // Need to compute the intersection: All missing entries are required....
      _Missing load_missing(cond_missing.size()+loaded.size());
      // Note: cond_missing is already sorted (doc of 'set_difference'). No need to re-sort....
      _Missing::iterator load_last = set_difference(begin(cond_missing), last_cond,
                                                    begin(loaded), end(loaded),
                                                    begin(load_missing), COMP());
      int num_load_miss = int(load_last-begin(load_missing));
      printout(num_load_miss==0 ? DEBUG : ERROR,"UserPool",
               "Found %ld out of %d conditions, which CANNOT be loaded...",
               num_load_miss, int(loaded.size()));

      for_each(loaded.begin(),loaded.end(),Inserter<MAPPING>(m_conditions,&m_iov));
      result.loaded  = slice_cond.size()-num_load_miss;
      result.missing = num_load_miss+num_calc_miss;
      if ( cond_missing.size() != loaded.size() )  {
        // ERROR!
      }
    }
  }
  //
  // Now we update the already existing dependencies, which have expired
  //
  if ( num_calc_miss > 0 )  {
    ConditionsDependencyCollection deps(calc_missing.begin(), last_calc, _to_dep);
    ConditionsDependencyHandler handler(m_manager, *this, deps, user_param);
    for(auto i=begin(deps); i != end(deps); ++i)   {
      const ConditionDependency* d = (*i).second.get();
      typename MAPPING::iterator j = m_conditions.find(d->key());
      // If we would know, that dependencies are only ONE level, we could skip this search....
      if ( j == m_conditions.end() )  {
        handler(d);
        continue;
      }
      // printout(INFO,"UserPool","Already calcluated: %s",d->name());
      continue;
    }
    result.computed = handler.num_callback;
    result.missing -= handler.num_callback;
  }
  return result;
}

namespace {
  template <typename MAPPING>
  void* create_pool(Geometry::LCDD&, int argc, char** argv)  {
    if ( argc > 1 )  {
      ConditionsManagerObject* m = (ConditionsManagerObject*)argv[0];
      ConditionsIOVPool* p = (ConditionsIOVPool*)argv[1];
      UserPool* pool = new ConditionsMappedUserPool<MAPPING>(m, p);
      return pool;
    }
    except("ConditionsMappedUserPool","++ Insufficient arguments: arg[0] = ConditionManager!");
    return 0;
  }
}

// Factory for the user pool using a binary tree map
void* create_map_user_pool(Geometry::LCDD& lcdd, int argc, char** argv)
{  return create_pool<std::map<Condition::key_type,Condition::Object*> >(lcdd, argc, argv);  }
DECLARE_LCDD_CONSTRUCTOR(DD4hep_ConditionsMapUserPool, create_map_user_pool)

// Factory for the user pool using a unordered map (hash-map)
void* create_unordered_map_user_pool(Geometry::LCDD& lcdd, int argc, char** argv)
{  return create_pool<std::unordered_map<Condition::key_type,Condition::Object*> >(lcdd, argc, argv);  }
DECLARE_LCDD_CONSTRUCTOR(DD4hep_ConditionsUnorderedMapUserPool, create_unordered_map_user_pool)

